#pragma once
#include <algorithm>
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

  // Called every time your operator needs to produce data. It processes the
  // input saved in `input_` and returns a new RowVector.
  RowVectorPtr getOutput() override {
    if(phase == 0 || input_ == nullptr) {
      return nullptr;
    }
    while(inputs.size() < 2) { // wait input
      std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    // We move `input_` to signal the input has been processed.
    auto currentInput = std::move(input_);

    std::vector<int64_t> firstResultColumn, secondResultColumn;
    auto buffer = currentInput->childAt(0)->template asFlatVector<int64_t>();
    auto buffer2 = currentInput->childAt(1)->template asFlatVector<int64_t>();

    // make sure the inputs are ordered correctly
    auto& input0 = inputNames[0].first == "c" ? inputs[0] : inputs[1];
    auto& input1 = inputNames[0].first != "c" ? inputs[0] : inputs[1];

    /* Initial Attempt */
    // std::sort input0 and input1
    // loop on index i = iterator for <c, d>, j = iterator for <a, b>
    //    if input1[j].second < input0[i].first
    //        increment j
    //    else if input1[j].second > input0[i].first
    //        increment i
    //    else
    //        hash join with currentInput(=input_)
    //            if hash value of input0[i] == currentInput[k]
    //                firstResultColumn = input1[j].first
    //                secondResultColumn = currentInput[k].second

    // Sort Phase
    auto sortPairByFirst = [] (auto entryOne, auto entryTwo) {
      if (entryOne.first == entryTwo.first)
        return entryOne.second < entryTwo.second;
      return entryOne.first < entryTwo.first;
    };

    auto sortPairBySecond = [] (auto entryOne, auto entryTwo) {
      if (entryOne.second == entryTwo.second)
        return entryOne.first < entryTwo.first;
      return entryOne.second < entryTwo.second;
    };

    // need to manually implement the sort function
    std::sort(input0.begin(), input0.end(), sortPairByFirst);  // <c, d>
    std::sort(input1.begin(), input1.end(), sortPairBySecond); // <a, b>

    auto leftI = 0;
    auto rightI = 0;

    // Hash Phase - Build
    // key = hashvalue, value = vector of pairs (for locality)
    std::vector<std::optional<std::vector<std::pair<int64_t, int64_t>>>> hashTable;
    auto modHash = [] (auto const& value) {
      // Data is in the 0~5000 range, hence there are no collisions
      // There will be some empty slots, but we're trading off memory for locality
      return value % 5000;
    };
    auto nextSlot = [&] (auto const& value) {
      return modHash(value + 1);
    };
    for (std::size_t i = 0; i < buffer->size(); ++i) {
      bool inserted = false;
      auto buildInput = buffer->valueAtFast(i), buffer2->valueAtFast(i)};
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


    // Sort-Merge Join on unique values
    // Merge Phase
    while (leftI < input1.size() && rightI < input0.size()) {
      auto leftInput = input1[leftI];
      auto rightInput = input0[rightI];
      if (leftInput.second < rightInput.first)
        leftI++;
      else if (leftInput.second > rightInput.first)
        rightI++;
      else {
        // Hash Phase - Probe and Join
        auto hashValue = modHash(rightInput.first);
        while (hashTable[hashValue].has_value() &&
               hashTable[hashValue].value()[0].first != rightInput.second)
          hashValue = nextSlot(hashValue);
        if (hashTable[hashValue].value()[0].first == rightInput.second) {
          for (auto const& entry : hashTable[hashValue].value()) {
            // Iterate through duplicates of the state where b == c and d == e
            firstResultColumn.push_back(leftInput.first); // a
            secondResultColumn.push_back(entry.second);   // f
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
