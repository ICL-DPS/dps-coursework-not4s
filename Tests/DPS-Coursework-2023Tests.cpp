#include "../Source/DPS-Coursework-2023.hpp"
#define CATCH_CONFIG_MAIN
#include "../Source/DataGenerator.hpp"
#include "../Source/MultiWayJoinPlan.hpp"
#include "../Source/ReferencePlan.hpp"
#include "../Source/Utilities.hpp"
#include <catch2/catch.hpp>

using namespace std;
using namespace datagenerator;
using namespace facebook::velox;
using namespace utilities;
using namespace plans;
using exec::test::CursorParameters;
using exec::test::TaskCursor;
namespace utilities {
std::shared_ptr<memory::MemoryPool> pool_{memory::getDefaultMemoryPool()};
}

namespace {
auto getReferenceResult(auto firstTable, auto secondTable, auto thirdTable) {
  auto firstValuesNode = utilities::makeValuesNode(planNodeIDGenerator, firstTable);
  auto secondValuesNode = utilities::makeValuesNode(planNodeIDGenerator, secondTable);
  auto thirdValuesNode = utilities::makeValuesNode(planNodeIDGenerator, thirdTable);

  auto reference = plans::makeReferencePlan(firstValuesNode, secondValuesNode, thirdValuesNode,
                                            planNodeIDGenerator);
  return getResult(reference);
}


auto getMultiWayResult(auto firstTable, auto secondTable, auto thirdTable) {
  auto reference = getMultiWayPlan(firstTable, secondTable, thirdTable);
  return getResult(reference);

  auto cursor = TaskCursor(CursorParameters({.planNode = reference}));
  std::unordered_multiset<pair<int64_t, int64_t>> referenceResult;
  while(cursor.moveNext()) {
    auto lastColumn = cursor.current()->childAt(0)->asFlatVector<int64_t>();
    auto firstColumn =
        cursor.current()->childAt(cursor.current()->childrenSize() - 1)->asFlatVector<int64_t>();
    for(auto i = 0u; i < cursor.current()->size(); i++) {
      referenceResult.emplace(firstColumn->valueAtFast(i), lastColumn->valueAtFast(i));
    }
  }
  return referenceResult;
}


} // namespace

namespace Catch {
template <> struct StringMaker<std::pair<int64_t, int64_t>> {
  static std::string convert(std::pair<int64_t, int64_t> const& value) {
    return "<" + std::to_string(value.first) + ", " + std::to_string(value.second) + ">";
  }
};
} // namespace Catch

TEST_CASE("Correctness on small random data") {

  auto firstTable = generateTable({"a", "b"}, 8, 2);
  auto secondTable = generateTable({"c", "d"}, 7, 3);
  auto thirdTable = generateTable({"e", "f"}, 6, 4);
  aggregate::prestosql::registerAllAggregateFunctions();

  auto referenceResult = getReferenceResult(firstTable, secondTable, thirdTable);
  auto multiWayResult = getMultiWayResult(firstTable, secondTable, thirdTable);

  REQUIRE(multiWayResult == referenceResult);
}

TEST_CASE("Correctness on larger random data") {
  auto firstTable = generateTable({"a", "b"}, 80, 8);
  auto secondTable = generateTable({"c", "d"}, 86, 9);
  auto thirdTable = generateTable({"e", "f"}, 37, 25);

  aggregate::prestosql::registerAllAggregateFunctions();

  auto referenceResult = getReferenceResult(firstTable, secondTable, thirdTable);
  auto multiWayResult = getMultiWayResult(firstTable, secondTable, thirdTable);

  REQUIRE(multiWayResult == referenceResult);
}

TEST_CASE("Correctness in context of plan") {
  auto firstTable = generateTable({"a", "b"}, 300, 8);
  auto secondTable = generateTable({"c", "d"}, 251, 9);
  auto thirdTable = generateTable({"e", "f"}, 47, 25);

  aggregate::prestosql::registerAllAggregateFunctions();

  auto referenceResult =
      getResult(makeCountByNode(getReferencePlan(firstTable, secondTable, thirdTable), "a"));
  auto multiWayResult =
      getResult(makeCountByNode(getMultiWayPlan(firstTable, secondTable, thirdTable), "a"));

  REQUIRE(multiWayResult == referenceResult);
}
