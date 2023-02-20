#include "../Source/DPS-Coursework-2023.hpp"
#include "../Source/DataGenerator.hpp"
#include "../Source/MultiWayJoinPlan.hpp"
#include "../Source/ReferencePlan.hpp"
#include "../Source/Utilities.hpp"
#include <benchmark/benchmark.h>
#include <iostream>
using namespace std;
using namespace datagenerator;
using namespace utilities;
using namespace plans;
namespace utilities {
std::shared_ptr<memory::MemoryPool> pool_{memory::getDefaultMemoryPool()};
}

static void ReferenceBenchmark(benchmark::State& state) {

  aggregate::prestosql::registerAllAggregateFunctions();

  for(auto _ : state) {
    state.PauseTiming(); // Stop timers. They will not count until they are resumed.
    auto firstTableScale = state.range(0);
    auto secondTableScale = (state.range(0) * 7) / 10;
    auto thirdTableScale = (state.range(0) * 5) / 100;
    auto firstTable = generateTable({"a", "b"}, firstTableScale, firstTableScale / 40);
    auto secondTable = generateTable({"c", "d"}, secondTableScale, secondTableScale / 50);
    auto thirdTable = generateTable({"e", "f"}, thirdTableScale, 320);
    auto plan = makeCountByNode(getReferencePlan(firstTable, secondTable, thirdTable), "f");
    state.ResumeTiming(); // Stop timers. They will not count until they are resumed.

    auto result = getResult(plan);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(ReferenceBenchmark)->Range(1024, 8 * 1024); // NOLINT

static void MultiWayBenchmark(benchmark::State& state) {

  aggregate::prestosql::registerAllAggregateFunctions();

  for(auto _ : state) {
    state.PauseTiming(); // Stop timers. They will not count until they are resumed.
    auto firstTableScale = state.range(0);
    auto secondTableScale = (state.range(0) * 7) / 10;
    auto thirdTableScale = (state.range(0) * 5) / 100;
    auto firstTable = generateTable({"a", "b"}, firstTableScale, firstTableScale / 40);
    auto secondTable = generateTable({"c", "d"}, secondTableScale, secondTableScale / 50);
    auto thirdTable = generateTable({"e", "f"}, thirdTableScale, 320);
    auto plan = makeCountByNode(getMultiWayPlan(firstTable, secondTable, thirdTable), "f");
    state.ResumeTiming(); // And resume timers. They are now counting again.

    auto result = getResult(plan);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(MultiWayBenchmark)->Range(1024, 8 * 1024); // NOLINT

BENCHMARK_MAIN();
