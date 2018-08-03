/*
 * gcc -std=c99 osc_shape.c -o osc_shape -lm
 *
 * ./osc_shape <frq>
 * aplay --rate=22050 --format=S16 osc_shape.s16.raw
 *
 * If we go up to 1.0, we just get a double pitch
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
static const Waveform wave_type[] = {SAW, SAW};

#define MAX_OSC 2
static float val[MAX_OSC], inc[MAX_OSC];

static void tick(float frq, float shift) {
  printf("note: frq=%f, shift=%f\n", frq, shift);
  for (uint16_t i = 0; i < MAX_OSC; i++) {
    val[i] = -1.0;
    inc[i] = 2.0 / ((float)AUDIO_RATE / frq);
  }
  val[1] += shift;
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
    }
    s = CLAMP(s, -1.0, 1.0);

    // convert to playback format
    buffer[i] = (SAMPLE)(SAMPLE_ZERO + ((s > 0.0)
        ? (s * (SAMPLE_AMP - 1))
        : (s * SAMPLE_AMP)));
    pos--;
  }
}

int main(int argc, char **argv) {
  const int buf_len = AUDIO_RATE; // 1 second
  SAMPLE buf[buf_len];
  FILE *out;
  float frq = 110.0;
  float shift;

  if (argc > 1) {
    frq = atof(argv[1]);
  }

  // write to file
  if ((out=fopen("osc_shape.s16.raw", "wb"))) {
    for (shift=0.0; shift<1.0;shift+=0.1) {
      tick(frq, shift);
      process(buf, buf_len);
      fwrite(buf, sizeof(SAMPLE), buf_len, out);
    }
    fclose(out);
  }
  return 0;
}
