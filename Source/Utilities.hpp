#pragma once
#include "velox/core/Expressions.h"
#include "velox/exec/tests/utils/AssertQueryBuilder.h"
#include "velox/exec/tests/utils/PlanBuilder.h"
#include "velox/expression/Expr.h"
#include "velox/functions/Udf.h"
#include "velox/functions/prestosql/aggregates/RegisterAggregateFunctions.h"
#include "velox/parse/Expressions.h"
#include <iostream>
#include <memory>
#include <tuple>
#include <utility>

#include "velox/common/file/FileSystems.h"
#include "velox/exec/tests/utils/Cursor.cpp"
#include "velox/expression/VectorFunction.h"

namespace utilities {

using namespace facebook::velox;
using namespace facebook::velox::core;
static auto planNodeIDGenerator = core::PlanNodeIdGenerator();
extern std::shared_ptr<memory::MemoryPool> pool_;

template <typename T> auto getField(std::string const name) {
  return std::make_shared<const FieldAccessTypedExpr>(CppToType<T>::create(), name);
}
static auto emptyFieldsList = std::vector<FieldAccessTypedExprPtr>{};

RowTypePtr concat(const RowTypePtr& a, const RowTypePtr& b) {
  std::vector<std::string> names = a->names();
  std::vector<TypePtr> types = a->children();
  names.insert(names.end(), b->names().begin(), b->names().end());
  types.insert(types.end(), b->children().begin(), b->children().end());
  return ROW(std::move(names), std::move(types));
}

template <typename... Ts> auto asITypedExprVector(Ts... things) {
  return std::vector<std::shared_ptr<const ITypedExpr>>{
      std::dynamic_pointer_cast<const ITypedExpr>(things)...};
}

template <typename T>
auto makeFlatVector(const std::vector<T>& data, const TypePtr& type = CppToType<T>::create()) {
  using TEvalType = typename CppToType<T>::NativeType;
  BufferPtr dataBuffer = AlignedBuffer::allocate<TEvalType>(data.size(), pool_.get());
  auto flatVector = std::make_shared<FlatVector<TEvalType>>(
      pool_.get(), type, BufferPtr(nullptr), data.size(), std::move(dataBuffer),
      std::vector<BufferPtr>(), SimpleVectorStats<TEvalType>{}, std::nullopt, (vector_size_t)0);
  for(vector_size_t i = 0; i < data.size(); i++) {
    flatVector->set(i, TEvalType(data[i]));
  }
  return flatVector;
}

RowVectorPtr makeRowVector(std::vector<std::string> childNames, std::vector<VectorPtr> children) {
  std::vector<std::shared_ptr<const Type>> childTypes;
  childTypes.resize(children.size());
  for(int i = 0; i < children.size(); i++) {
    childTypes[i] = children[i]->type();
  }
  auto rowType = ROW(std::move(childNames), std::move(childTypes));
  const size_t vectorSize = children.empty() ? 0 : children.front()->size();

  return std::make_shared<RowVector>(pool_.get(), rowType, BufferPtr(nullptr), vectorSize,
                                     children);
}

auto makeValuesNode(auto& planNodeIDGenerator, RowVectorPtr values) {
  return std::make_shared<core::ValuesNode>(planNodeIDGenerator.next(),
                                            std::vector<RowVectorPtr>{values}, false);
}

auto makeCountByNode(core::PlanNodePtr input, std::string groupingAttribute = "a") {
  auto aggregationCallExpression = make_shared<const core::CallTypedExpr>(
      CppToType<int64_t>::create(), asITypedExprVector(), "count");

  return std::make_shared<core::AggregationNode>(
      planNodeIDGenerator.next(), core::AggregationNode::Step::kSingle,
      std::vector{getField<int64_t>(groupingAttribute)}, emptyFieldsList,
      std::vector{std::string("f")}, std::vector{aggregationCallExpression}, emptyFieldsList,
      true, std::move(input));
}

auto getResult(PlanNodePtr reference) {
  using namespace facebook::velox::exec::test;
  auto cursor = TaskCursor(CursorParameters({.planNode = reference}));
  std::unordered_multiset<std::pair<int64_t, int64_t>> referenceResult;
  while(cursor.moveNext()) {
    auto names = dynamic_pointer_cast<const RowType>(cursor.current()->type())->names();
    std::map<std::string, decltype(cursor.current()->childAt(0)->asFlatVector<int64_t>())> columns;
    auto i = 0;
    for (auto& name : names) {
      columns[name]= cursor.current()->childAt(i++)->asFlatVector<int64_t>();
    }
    for(auto i = 0u; i < cursor.current()->size(); i++) {
      referenceResult.emplace(columns["a"]->valueAtFast(i), columns["f"]->valueAtFast(i));
    }
  }
  return referenceResult;
}

} // namespace utilities
