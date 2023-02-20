#pragma once
#include "Utilities.hpp"
#include <string>
#include <vector>
namespace datagenerator {
using namespace facebook::velox;
using namespace facebook::velox::core;
using namespace utilities;

RowVectorPtr generateTable(std::vector<std::string> names, size_t size = 501000,
                           size_t uniqueValues = 5000) {
  std::default_random_engine generator;
  std::uniform_int_distribution<int64_t> distribution(1, uniqueValues);

  auto firstColumn = std::vector<int64_t>();
  auto secondColumn = std::vector<int64_t>();
  firstColumn.reserve(size);
  secondColumn.reserve(size);
  for(auto i = 0u; i < size; i++) {
    firstColumn.push_back(distribution(generator));
    secondColumn.push_back(distribution(generator));
  }
  return makeRowVector(
                names, {makeFlatVector<int64_t>(firstColumn), makeFlatVector<int64_t>(secondColumn)});
}
} // namespace datagenerator
