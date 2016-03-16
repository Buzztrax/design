/*
 * gcc -std=c99 osc.c -o osc -lm
 *
 * ./osc
 * aplay --rate=22050 --format=S16 osc.s16.raw
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

static const int AUDIO_RATE = 22050;
static const int16_t SAMPLE_MIN = -(1 << 15);
static const int16_t SAMPLE_ZERO = 0;
static const int16_t SAMPLE_MAX = ((1 << 15) - 1);
static const int16_t SAMPLE_AMP = (1 << 15);
static const Waveform wave_type[] = {SAW, SAW, SAW};
static const int detune_coarse[] = {0, 4, 7};
static const int detune_fine[] = {0, 0, 0};

#define MAX_OSC 3
static float val[MAX_OSC], inc[MAX_OSC], inc2[MAX_OSC];

static void tick(float frq) {
  printf("note: frq=%f\n", frq);
  for (uint16_t i = 0; i < MAX_OSC; i++) {
    float d = pow (2.0, (detune_coarse[i] / 12.0)) * pow (2.0, (detune_fine[i] / 1200.0));
    val[i] = -1.0;
    inc[i] = 2.0 / ((float)AUDIO_RATE / (frq * d));
    inc2[i] = 0.0;
    printf("d=%7.4f, %+2d %+3d\n", d, detune_coarse[i], detune_fine[i]);
  }
}

static void process(SAMPLE *buffer, uint16_t len) {
  float vol, s;
  uint16_t pos = len;

  for (uint16_t i = 0; i < len; i++) {
    /*if (i%255) {
      float v = fmod(i * inc[0], 2.0) - 1.0;
      printf("%5d: %+9.7f == %+9.7f : %9.7f\n", i, val[0], v, val[0] - v);
    }*/

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
      inc[j] += inc2[j];
    }
    s = CLAMP(s, -1.0, 1.0);

    // convert to playback format
    buffer[i] = SAMPLE_ZERO + ((s > 0.0)
        ? (SAMPLE)(s * (SAMPLE_AMP - 1))
        : (SAMPLE)(s * SAMPLE_AMP));
    pos--;
  }
}

int main(int argc, char **argv) {
  const int buf_len = AUDIO_RATE; // 1 second
  SAMPLE buf[buf_len];
  FILE *out;
  float frq = 110.0;

  if (argc > 1) {
    frq = atof(argv[1]);
  }

  tick(frq);
  process(buf, buf_len);

  // write to file
  if ((out=fopen("osc.s16.raw", "wb"))) {
    fwrite(buf, sizeof(SAMPLE), buf_len, out);
    fclose(out);
  }
  return 0;
}