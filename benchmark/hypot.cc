/* g++ -pthread -std=c++11 hypot.cc -lm -lbenchmark -o hypot
 *
 * sudo cpupower frequency-set --governor performance; ./hypot; sudo cpupower frequency-set --governor powersave
 */

#include <benchmark/benchmark.h>
#include <math.h>

static volatile double xf = 3.0, yf = 5.0;

static void BM_SqrtMulAddF(benchmark::State& state) {
  for (auto _ : state)
    benchmark::DoNotOptimize(sqrtf(xf * xf + yf * yf));
}
BENCHMARK(BM_SqrtMulAddF);

static void BM_HypotF(benchmark::State& state) {
  for (auto _ : state)
    benchmark::DoNotOptimize(hypotf(xf, yf));
}
BENCHMARK(BM_HypotF);

static volatile double xd = 3.0, yd = 5.0;

static void BM_SqrtMulAddD(benchmark::State& state) {
  for (auto _ : state)
    benchmark::DoNotOptimize(sqrt(xd * xd + yd * yd));
}
BENCHMARK(BM_SqrtMulAddD);

static void BM_HypotD(benchmark::State& state) {
  for (auto _ : state)
    benchmark::DoNotOptimize(hypot(xd, yd));
}
BENCHMARK(BM_HypotD);

BENCHMARK_MAIN();
