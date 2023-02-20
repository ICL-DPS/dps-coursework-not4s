#include "Utilities.hpp"

namespace plans {
auto makeReferencePlan(auto inputValuesNode, auto secondInputValuesNode, auto thirdInputValuesNode,
                       auto& planNodeIDGenerator) {
  using namespace facebook::velox;
  using namespace facebook::velox::core;
  using facebook::velox::core::ITypedExpr;
  using namespace utilities;

  auto joinNode = std::shared_ptr<core::HashJoinNode>(new core::HashJoinNode(
      planNodeIDGenerator.next(), core::JoinType::kInner, false,
      std::vector{getField<int64_t>("b")}, std::vector{getField<int64_t>("c")}, {}, inputValuesNode,
      secondInputValuesNode,
      concat(inputValuesNode->outputType(), secondInputValuesNode->outputType())));

  auto secondJoinNode = std::shared_ptr<core::HashJoinNode>(new core::HashJoinNode(
      planNodeIDGenerator.next(), core::JoinType::kInner, false, {getField<int64_t>("d")},
      {getField<int64_t>("e")}, {}, joinNode, thirdInputValuesNode,
      concat(joinNode->outputType(), thirdInputValuesNode->outputType())));
  return secondJoinNode;
}

auto getReferencePlan(auto firstTable, auto secondTable, auto thirdTable) {
  auto firstValuesNode = utilities::makeValuesNode(planNodeIDGenerator, firstTable);
  auto secondValuesNode = utilities::makeValuesNode(planNodeIDGenerator, secondTable);
  auto thirdValuesNode = utilities::makeValuesNode(planNodeIDGenerator, thirdTable);

  return plans::makeReferencePlan(firstValuesNode, secondValuesNode, thirdValuesNode,
                                  planNodeIDGenerator);
}

} // namespace plans
