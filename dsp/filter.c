/*
 * gcc -std=c99 filter.c -o filter -lm
 *
 * ./filter
 * aplay --rate=22050 --format=S16 filter.s16.raw
 */

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define CLAMP(v,l,u) (((v) > (u)) ? (u) : (((v) < (l)) ? (l) : (v)))

typedef int16_t SAMPLE;
typedef enum  {
  SAW = 0,
  SQR,
  TRI
} Waveform;

typedef enum  {
  LOW = 0,
  MID,
  HIGH,
  FILTER_SIZE
} Filter;

static const int AUDIO_RATE = 22050;
static const int16_t SAMPLE_MIN = -(1 << 15);
static const int16_t SAMPLE_ZERO = 0;
static const int16_t SAMPLE_MAX = ((1 << 15) - 1);
static const int16_t SAMPLE_AMP = (1 << 15);
static const char * wave_name[] = {"saw", "sqr", "tri"};
static const char * flt_name[] = {"low", "mid", "high"};

#define MAX_OSC 1
static float val[MAX_OSC] = {-1.0, }, inc[MAX_OSC];
static const Waveform wave_type[MAX_OSC] = {SQR, };
#define MAX_SLOPE 4
static float flt[FILTER_SIZE * MAX_SLOPE];
static float cutoff = 0.5, resonance = 1.0;
static Filter flt_type = LOW;
static float mi[MAX_SLOPE] = {1.0, }, ma[MAX_SLOPE] = {-1.0, };

static void process(SAMPLE *buffer, uint16_t len) {
  float vol, cut, res, s;
  float *f;
  uint16_t pos = len;

  for (uint16_t i = 0; i < len; i++) {
    vol = ((float)pos) / ((float)len);
    vol = (vol * vol) / (float)MAX_OSC;
    // osc + decay env
    s = 0.0;
    for (uint16_t j = 0; j < MAX_OSC; j++) {
      switch (wave_type[j]) {
        case SAW:
          s += val[j] * vol;
          break;
        case SQR:
          s += (val[j] > 0.0) ? vol : -vol;
          break;
        case TRI:
          if (val[j] > 0.5) {
            s += vol * 2.0 * (0.5 - val[j]);
          } else if (val[j] < -0.5) {
            s += vol * 2.0 * (-0.5 - val[j]);
          } else {
            s += vol * 2.0 * val[j];
          }
          break;
        default:
          break;
      }
      val[j] += inc[j];
      if (val[j] > 1.0) {
          val[j] -= 2.0;
      }
    }

    cut = cutoff;
    res = 1.0 / resonance;
    // cascade filters for extra steepness
    f = flt;
    for (uint8_t j = 0; j < MAX_SLOPE; j++) {
      f[HIGH] = s - (f[MID] * res) - f[LOW];
      f[MID] += (f[HIGH] * cut);
      f[LOW] += (f[MID] * cut);
      s = f[flt_type];

      // track min/max amp at each stage
      if (s < mi[j]) {
        mi[j] = s;
      } else if (s > ma[j]) {
        ma[j] = s;
      }

      // TODO: need softclipping
      //s = CLAMP(s, -1.0, 1.0);  // clip here or outside?
      f += FILTER_SIZE; // switch to next filter state
    }

    // convert to playback format
    /*
    buffer[i] = (SAMPLE)(SAMPLE_ZERO + ((s > 0.0)
        ? (s * (SAMPLE_AMP - 1))
        : (s * SAMPLE_AMP)));
    */
    int32_t s2 = (SAMPLE_ZERO + ((s > 0.0)
        ? (s * (SAMPLE_AMP - 1))
        : (s * SAMPLE_AMP)));
    buffer[i] = CLAMP(s2, SAMPLE_MIN, SAMPLE_MAX);

    pos--;
  }
}

static void save_sample(const char *name, SAMPLE *buffer, uint16_t len) {
  FILE *out;

  if ((out=fopen("filter.s16.raw", "wb"))) {
    fwrite(buffer, sizeof(SAMPLE), len, out);
    fclose(out);
  }
}

/* add a gtk ui with cairo drawable
 * draw x=osc-frq, y=cut-off frequency, gray is volume
 * draw 3 images for slope={1,2,3}
 *
 * when clicking a point, generate wave, show below and play
 *
 * parameters for osc, filter type
 */

int main(int argc, char **argv) {
  const int buf_len = AUDIO_RATE; // 1 second
  SAMPLE buf[buf_len];
  float frq = 10.0;

  for (int i=0; i<argc; i++) {
    switch (i) {
      case 1:
        cutoff = atof(argv[i]);
        break;
      case 2:
        resonance = atof(argv[i]);
        break;
    }
  }
  printf("# wave=%s, filter=%s, cutoff=%f, resonance=%f\n",
      wave_name[wave_type[0]], flt_name[flt_type], cutoff, resonance);

  while(frq < AUDIO_RATE / 2) {
    for (uint8_t i = 0; i < MAX_OSC; i++) {
      inc[i] = 2.0 / ((float)AUDIO_RATE / frq);
    }
    process(buf, buf_len);

    // report volumes
    printf("%7.1f", frq);
    for (uint8_t j = 1; j < MAX_SLOPE; j++) {
      float fcmi = mi[j] / mi[j-1], fcma = ma[j] / ma[j-1];
      printf(", %7.4f", ((fcmi > fcma) ? fcmi : fcma));
    }
    putchar('\n');
    frq+=frq;
  }

  // write to file
  if (0) save_sample("filter.s16.raw", buf, buf_len);

  return 0;
}