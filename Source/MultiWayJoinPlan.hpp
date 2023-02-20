#pragma once
#include "Utilities.hpp"
#include "ThreeWayJoin.hpp"
#include <chrono>
#include <thread>

namespace plans {
using namespace facebook::velox;
using namespace datagenerator;

auto makeMultiWayPlan(auto inputValuesNode, auto secondInputValuesNode, auto thirdInputValuesNode,
                      auto& planNodeIDGenerator) {
  exec::Operator::registerOperator(std::make_unique<MultiWayJoinTranslator>());

  return std::make_shared<MultiWayJoinNode>(
      planNodeIDGenerator.next(),
      std::vector<core::PlanNodePtr>{inputValuesNode, secondInputValuesNode, thirdInputValuesNode});
}
auto getMultiWayPlan(auto firstTable, auto secondTable, auto thirdTable) {
  auto firstValuesNode = utilities::makeValuesNode(planNodeIDGenerator, firstTable);
  auto secondValuesNode = utilities::makeValuesNode(planNodeIDGenerator, secondTable);
  auto thirdValuesNode = utilities::makeValuesNode(planNodeIDGenerator, thirdTable);

  return plans::makeMultiWayPlan(firstValuesNode, secondValuesNode, thirdValuesNode,
                                 planNodeIDGenerator);
}

} // namespace plans
