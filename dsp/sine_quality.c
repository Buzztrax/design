/* Quality comparision for sine functions
 *
 * build:
 * gcc sine_quality.c -I. -lm -o sine_quality
 *
 * run without args:
 * ./sine_quality
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <linux/limits.h>

#include "sines.h"

// TODO: make a parameter or run for 10 Hz, 100 Hz, 1000Hz
// cycle_length = 44100 / frq
const double STEPS = 1024;  // 44100 / 1024 ~ 43 Hz

// Error tracking helper

typedef struct {
  double a;  // aggregate
  double m;  // max
  const char *fn; // function name
  FILE *ef;
} Error;
static Error e;

static void error_init(Error *e, const char *fn ) {
  char fname[PATH_MAX];
  e->a = e->m = 0.0;
  e->fn = fn;

  snprintf(fname, PATH_MAX, "%s.s16.raw", fn);
  e->ef = fopen(fname, "wb");
}

static void error_update(Error *e, float c, float a) {
  float ec = fabs(c - sinf(a));
  e->a += ec;
  if (ec > e->m) {
    e->m = ec;
  }

  // scale up, otherwise we don't see anything
  int16_t sample = (int16_t) (ec * 100 * 32768.0);
  fwrite(&sample, sizeof(int16_t), 1, e->ef);
}

static void error_report(Error *e) {
  fclose(e->ef);
  printf("%15s : avg=%lf, max=%lf\n", e->fn, e->a / STEPS, e->m);
}

// FastSine --------------------------------------------------------------------

static void QM_FastSineF(void) {
  float angle = M_PI / STEPS;
  float sk = 0.0, ck = 1.0, skk, ckk;
  float s1 = sin(angle);
  float c1 = cos(angle);
  float a = 0.0;
  error_init(&e,  __FUNCTION__);
  for (int i = 0; i < STEPS; i++) {
    skk = c1 * sk + s1 * ck;
    ck = c1 * ck - s1 * sk;
    sk = skk;
    error_update(&e, sk, a);
    a += angle;
  }
  error_report(&e);
}

// FasterSine ------------------------------------------------------------------

static void QM_FasterSineF(void) {
  float angle = M_PI / STEPS;
  float si0 = sinf(-angle);
  float si1 = sinf(0.0);
  float fc = 2.0 * cosf(angle);
  float a = 0.0;
  error_init(&e,  __FUNCTION__);
  for (int i = 0; i < STEPS; i++) {
    si0 = fc * si1 - si0;
    error_update(&e, si0, a);
    float t = si0; si0 = si1; si1 = t;
    a += angle;
  }
  error_report(&e);
}

// Quarter Table Sine ----------------------------------------------------------

static void QM_QTabSineF(void) {
  const float rad2brad = 256.0 / (2.0 * M_PI);
  float angle = (M_PI / STEPS);
  float a = 0.0;
  error_init(&e,  __FUNCTION__);
  for (int i = 0; i < STEPS; i++) {
    error_update(&e, qtab_sin(a * rad2brad), a);
    a += angle;
  }
  error_report(&e);
}

// Full Table Sine -------------------------------------------------------------

static void QM_FTabSineF(void) {
  const float rad2brad = 256.0 / (2.0 * M_PI);
  float angle = (M_PI / STEPS);
  float a = 0.0;
  error_init(&e,  __FUNCTION__);
  for (int i = 0; i < STEPS; i++) {
    error_update(&e, ftab_sin(a * rad2brad), a);
    a += angle;
  }
  error_report(&e);
}

static void QM_FTabSineFInt(void) {
  const float rad2brad = 256.0 / (2.0 * M_PI);
  float angle = (M_PI / STEPS);
  float a = 0.0;
  error_init(&e,  __FUNCTION__);
  for (int i = 0; i < STEPS; i++) {
    error_update(&e, ftab_sin_int(a * rad2brad), a);
    a += angle;
  }
  error_report(&e);
}


void main(void) {
  QM_FastSineF();
  QM_FasterSineF();
  QM_QTabSineF();
  QM_FTabSineF();
  QM_FTabSineFInt();
}