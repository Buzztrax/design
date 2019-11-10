/* Microbenchmark for sine series
 *
 * build:
 * requires https://github.com/google/benchmark
 *
 * export BENCH_DIR=$HOME/projects/tools/benchmark
 * # without optimizations
 * g++ sine_bench.cpp -std=c++11 -isystem $BENCH_DIR/include -I. -L$BENCH_DIR/build/src -lbenchmark -lpthread -o sine_bench
 * # with optimizations
 * g++ -O2 -march=native sine_bench.cpp -std=c++11 -isystem $BENCH_DIR/include -I. -L$BENCH_DIR/build/src -lbenchmark -lpthread -o sine_bench
 *
 * run without args:
 * ./sine_bench
 *
 * observations:
 * -O3: the _X2 functions get slower?)
 * -ftree-vectorize:  BM_FastSineD becomes faster than BM_FastSineF
 */

#include <benchmark/benchmark.h>
#include <stdint.h>
#include <math.h>

#include <sines.h>

const double STEPS = 32;

// Baseline (from libm) --------------------------------------------------------

static void BM_MathSineD(benchmark::State& state) {
  double angle = M_PI / STEPS;
  for (auto _ : state) {
    double a = 0.0;
    for (int i = 0; i < STEPS; i++) {
      benchmark::DoNotOptimize(sin(a));
      a += angle;
    }
  }
}

BENCHMARK(BM_MathSineD);

static void BM_MathSineF(benchmark::State& state) {
  float angle = M_PI / STEPS;
  for (auto _ : state) {
    float a = 0.0;
    for (int i = 0; i < STEPS; i++) {
      benchmark::DoNotOptimize(sinf(a));
      a += angle;
    }
  }
}

BENCHMARK(BM_MathSineF);

// FastSine --------------------------------------------------------------------

static void BM_FastSineD(benchmark::State& state) {
  double angle = M_PI / STEPS;
  for (auto _ : state) {
    double sk = 0.0, ck = 1.0, skk, ckk;
    double s1 = sin(angle);
    double c1 = cos(angle);
    for (int i = 0; i < STEPS; i++) {
      benchmark::DoNotOptimize(skk = c1 * sk + s1 * ck);
      benchmark::DoNotOptimize(ck = c1 * ck - s1 * sk);
      sk = skk;
    }
  }
}

BENCHMARK(BM_FastSineD);

static void BM_FastSineF(benchmark::State& state) {
  float angle = M_PI / STEPS;
  for (auto _ : state) {
    float sk = 0.0, ck = 1.0, skk, ckk;
    float s1 = sin(angle);
    float c1 = cos(angle);
    for (int i = 0; i < STEPS; i++) {
      benchmark::DoNotOptimize(skk = c1 * sk + s1 * ck);
      benchmark::DoNotOptimize(ck = c1 * ck - s1 * sk);
      sk = skk;
    }
  }
}

BENCHMARK(BM_FastSineF);

// FasterSine ------------------------------------------------------------------

static void BM_FasterSineD(benchmark::State& state) {
  double angle = M_PI / STEPS;
  for (auto _ : state) {
    double si0 = sin(-angle);
    double si1 = sin(0.0);
    double fc = 2.0 * cos(angle);
    for (int i = 0; i < STEPS; i++) {
      benchmark::DoNotOptimize(si0 = fc * si1 - si0);
      std::swap(si0, si1);
    }
  }
}

BENCHMARK(BM_FasterSineD);

static void BM_FasterSineF(benchmark::State& state) {
  float angle = M_PI / STEPS;
  for (auto _ : state) {
    float si0 = sinf(-angle);
    float si1 = sinf(0.0);
    float fc = 2.0 * cosf(angle);
    for (int i = 0; i < STEPS; i++) {
      benchmark::DoNotOptimize(si0 = fc * si1 - si0);
      std::swap(si0, si1);
    }
  }
}

BENCHMARK(BM_FasterSineF);

// Version that is unrolled by *2 to save the swap operation
static void BM_FasterSineF_X2(benchmark::State& state) {
  float angle = M_PI / STEPS;
  for (auto _ : state) {
    float si0 = sinf(-angle);
    float si1 = sinf(0.0);
    float fc = 2.0 * cosf(angle);
    for (int i = 0; i < STEPS; i+=2) {
      benchmark::DoNotOptimize(si0 = fc * si1 - si0);
      benchmark::DoNotOptimize(si1 = fc * si0 - si1);
    }
  }
}

BENCHMARK(BM_FasterSineF_X2);

// Quarter Table Sine ----------------------------------------------------------

static void BM_QTabSineF(benchmark::State& state) {
  const float rad2brad = 255.0 / (2.0 * M_PI);
  float angle = (M_PI / STEPS) * rad2brad;
  for (auto _ : state) {
    float a = 0.0;
    for (int i = 0; i < STEPS; i++) {
      benchmark::DoNotOptimize(qtab_sin(a));
      a += angle;
    }
  }
}

BENCHMARK(BM_QTabSineF);

// Full Table Sine -------------------------------------------------------------

static void BM_FTabSineF(benchmark::State& state) {
  const float rad2brad = 256.0 / (2.0 * M_PI);
  float angle = (M_PI / STEPS) * rad2brad;
  for (auto _ : state) {
    float a = 0.0;
    for (int i = 0; i < STEPS; i++) {
      benchmark::DoNotOptimize(ftab_sin(a));
      a += angle;
    }
  }
}

BENCHMARK(BM_FTabSineF);

static void BM_FTabSineFInt(benchmark::State& state) {
  const float rad2brad = 256.0 / (2.0 * M_PI);
  float angle = (M_PI / STEPS) * rad2brad;
  for (auto _ : state) {
    float a = 0.0;
    for (int i = 0; i < STEPS; i++) {
      benchmark::DoNotOptimize(ftab_sin_int(a));
      a += angle;
    }
  }
}

BENCHMARK(BM_FTabSineFInt);


BENCHMARK_MAIN();
