#pragma once
namespace {
using namespace facebook::velox;
using namespace datagenerator;

class MultiWayJoinNode : public core::PlanNode {
public:
  MultiWayJoinNode(const core::PlanNodeId& id, std::vector<core::PlanNodePtr> sources)
      : PlanNode(id), sources_{sources} {}

  // Output type is the type of the first input
  const RowTypePtr& outputType() const override {
    static auto type =
        ROW({"a", "f"}, {CppToType<int64_t>::create(), CppToType<int64_t>::create()});
    return type;  
  }

  const std::vector<core::PlanNodePtr>& sources() const override { return sources_; }

  std::string_view name() const override { return "three way join"; }

private:
  // One can add details about the plan node and its metadata in a textual
  // format.
  void addDetails(std::stringstream& /* stream */) const override {}

  std::vector<core::PlanNodePtr> sources_;
};

static std::vector<std::vector<std::pair<int64_t, int64_t>>> inputs;
static std::vector<std::pair<std::string, std::string>> inputNames;

// Second, let's define the operator. Here's where the main logic lives.
template <int phase> class MultiWayJoinOperator : public exec::Operator {
public:
  // The operator takes the plan node defined above, which could contain
  // additional metadata.
  MultiWayJoinOperator(int32_t operatorId, exec::DriverCtx* driverCtx,
                       std::shared_ptr<const MultiWayJoinNode> planNode)
      : Operator(driverCtx, nullptr, operatorId, planNode->id(), "DuplicateRow") {}

  // Called every time there's input available. We just save it in the `input_`
  // member defined in the base class, and process it on `getOutput()`.
  void addInput(RowVectorPtr input) override {
    if(phase == 0 && input) {
      auto buffer = input->childAt(0)->asFlatVector<int64_t>();
      auto buffer2 = input->childAt(1)->asFlatVector<int64_t>();
      std::vector<std::pair<int64_t, int64_t>> table;
      for(auto i = 0u; i < buffer->size(); i++) {
        table.emplace_back(buffer->valueAtFast(i), buffer2->valueAtFast(i));
      }
      static std::mutex input2Mutex;
      input2Mutex.lock();
      // keep the names of the input columns
      inputNames.emplace_back(dynamic_pointer_cast<const RowType>(input->type())->names().at(0),
                              dynamic_pointer_cast<const RowType>(input->type())->names().at(1));
      inputs.push_back(std::move(table));
      input2Mutex.unlock();
    }
    input_ = input;
  }

  bool needsInput() const override { return !noMoreInput_; }

  void quickSort(std::vector<std::pair<int64_t,int64_t>> &arr, int64_t low, int64_t high, bool sortByFirst) {
    if (low < high) {
        int64_t i = low;
        int64_t j = high;
        int64_t pivot = sortByFirst ? arr[(low + high) / 2].first : arr[(low + high) / 2].second;
        while (i <= j) {
          int64_t currI = sortByFirst ? arr[i].first : arr[i].second;
          int64_t currJ = sortByFirst ? arr[j].first : arr[j].second;
          
            while (currI < pivot) {
                currI = sortByFirst ? arr[++i].first : arr[++i].second;
            }
            while (currJ > pivot) {
                currJ = sortByFirst ? arr[--j].first : arr[--j].second ;
            }
            if (i <= j) {
                swap(arr[i], arr[j]);
                i++;
                j--;
            }
        }
        if (low < j) {
            quickSort(arr, low, j, sortByFirst);
        }
        if (i < high) {
            quickSort(arr, i, high, sortByFirst);
        }
    }
  }

  // Called every time your operator needs to produce data. It processes the
  // input saved in `input_` and returns a new RowVector.
  RowVectorPtr getOutput() override {
    if (phase == 0 || input_ == nullptr) {
      return nullptr;
    }
    while (inputs.size() < 2) { // wait input
      std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    // We move `input_` to signal the input has been processed.
    auto currentInput = std::move(input_);

    std::vector<int64_t> firstResultColumn, secondResultColumn;
    auto buffer = currentInput->childAt(0)->template asFlatVector<int64_t>();
    auto buffer2 = currentInput->childAt(1)->template asFlatVector<int64_t>();

    // Make sure the inputs are ordered correctly
    auto& input0 = inputNames[0].first == "c" ? inputs[0] : inputs[1]; // < c, d >
    auto& input1 = inputNames[0].first != "c" ? inputs[0] : inputs[1]; // < e, f >

    // Sort Phase
    std::vector<std::pair<int64_t, int64_t>> input2(buffer->size());
    for (size_t i = 0; i < buffer->size(); ++i) {
      input2[i] = { buffer->valueAtFast(i), buffer2->valueAtFast(i) };
    }

    quickSort(input0, 0, input0.size() - 1, true);
    quickSort(input2, 0, input2.size() - 1, false);

    // Hash Phase - Build
    // key = hashvalue, value = vector of pairs (for locality)
    std::vector<std::optional<std::vector<std::pair<int64_t, int64_t>>>> hashTable(100);
    auto modHash = [] (auto const& value) {
      return value % 100;
    };

    auto nextSlot = [&] (auto const& value) {
      return modHash(value + 1);
    };

    // Hash the <e,f> table
    for (std::size_t i = 0; i < input1.size(); ++i) {
      bool inserted = false;
      std::pair<int64_t, int64_t> buildInput = input1[i];
      auto hashValue = modHash(buildInput.first);
      
      while (hashTable[hashValue].has_value()) {
        if (buildInput.first == hashTable[hashValue].value()[0].first) {
          hashTable[hashValue].value().push_back(buildInput);
          inserted = true;
          break;
        }
        hashValue = nextSlot(hashValue);
      }
      if (!inserted) hashTable[hashValue] = {buildInput};
    }

    // Merge phase
    for (auto const& rightInput : input0) {
      auto leftI = 0;
      auto hashValue = modHash(rightInput.second);

      while (hashTable[hashValue].has_value() &&
               hashTable[hashValue].value()[0].first != rightInput.second)
          hashValue = nextSlot(hashValue);

      if (hashTable[hashValue].has_value() && hashTable[hashValue].value()[0].first == rightInput.second) {
        while (leftI < input2.size()) {
          auto leftInput = input2[leftI];

          if (leftInput.second < rightInput.first) {
            leftI++;
          } else if (leftInput.second > rightInput.first) {
            break;
          } else {
            for (auto const& entry : hashTable[hashValue].value()) {
              firstResultColumn.push_back(leftInput.first); // a
              secondResultColumn.push_back(entry.second);   // f
            }
            leftI++;
          }
        }
      }
    }
    
    inputs.clear();
    if(firstResultColumn.size() == 0)
      return nullptr;
    return makeRowVector({"a", "f"}, {makeFlatVector<int64_t>(firstResultColumn),
                                      makeFlatVector<int64_t>(secondResultColumn)});
  }

  // This simple operator is never blocked.
  exec::BlockingReason isBlocked(ContinueFuture* future) override {
    return exec::BlockingReason::kNotBlocked;
  }

  bool isFinished() override { return !needsInput(); }
};

// Third, we need to define a plan translation logic to convert our custom plan
// node into our custom operator. Check `velox/exec/LocalPlanner.cpp` for more
// details.
class MultiWayJoinTranslator : public exec::Operator::PlanNodeTranslator {
  std::unique_ptr<exec::Operator> toOperator(exec::DriverCtx* ctx, int32_t id,
                                             const core::PlanNodePtr& node) override {
    if(auto dupRowNode = std::dynamic_pointer_cast<const MultiWayJoinNode>(node)) {
      return std::make_unique<MultiWayJoinOperator<1>>(id, ctx, dupRowNode);
    }
    return nullptr;
  }

  exec::OperatorSupplier toOperatorSupplier(const core::PlanNodePtr& node) override {
    if(auto dupRowNode = std::dynamic_pointer_cast<const MultiWayJoinNode>(node)) {
      return [dupRowNode](int32_t id, exec::DriverCtx* ctx) {
        return std::make_unique<MultiWayJoinOperator<0>>(id, ctx, dupRowNode);
      };
    }
    return nullptr;
  };
};
} // namespace