/*
 * oscparse: parse and dump osc messages
 *
 * gcc oscparse.c -o oscparse
 *
 * tcp:
 * nc -l localhost 7930 | oscparse
 * oscsend osc.tcp://localhost:7930 /synth/mono/filter/resonance f 1.5
 *
 * udp:
 * nc -lu localhost 7930 | oscparse
 * oscsend osc.udp://localhost:7930 /synth/mono/filter/resonance f 1.5
 */

#include <stdio.h>
#include <string.h>

static void hexdump(char *buf, int len) {
  int i = 0;
  char ascii[33]={0,}, *ap = ascii;

  while (i < len) {
    *ap++ = (buf[i]>32) ? buf[i] : '.'; *ap = '\0';
    if (!(i&0x1F)) {
      if (!i) printf("%08x  ",i);
      else {
        printf("  %s\n%08x  ",ascii, i);
        ap = ascii;
      }
    }
    else if (!(i&0x7)) printf(" ");
    printf("%02x ", buf[i]);
    i++;
  }
  printf("  %s\n", ascii);
}

static int round_up_4bytes(int p) {
  if (p & 0x3) {
    p = ((p >> 2) + 1) << 2;
  }
  return p;
}

static char swap[4];
static char *swap_4bytes(const char *s, char *d) {
  d[0] = s[3];d[1] = s[2];d[2] = s[1];d[3] = s[0];
  return d;
}

static void parse(char *msg) {
  int i = 0, j, k;
  union _v {
    int i;
    float f;
    char *s;
    int b; // T/F
    // need more
  } value[10];
  char *address, *format;
  // debug (pos, len)
  int ap = 0, al, fp, fl;
  // debug

  address = msg;
  if (address[0] != '/') {
    printf("osc: malformed message, address must start with '/'\n");
    goto Error;
  }
  j = strlen(msg);          // end of addr
  j = round_up_4bytes(j + 1);
  // debug
  al = j - ap;
  printf("Address okay: pos=%3d, len=%3d, '%s'\n", ap, al, address);
  // debug
  if (msg[j] != ',') {
    printf("osc: malformed message, format must start with ','\n");
    goto Error;
  }
  format = &msg[j+1]; // skip ','
  k = strlen(format);
  j +=  1 + k;  // end of fmt
  j = round_up_4bytes(j + 1);
  if (k > 10) {
    printf("osc: too many args: %d > 10\n", k);
    goto Error;
  }
  // debug
  fp = ap + al;
  fl = j - fp;
  printf("Format okay: pos=%3d, len=%3d, '%s'\n", fp, fl, format);
  // debug
  // parse args
  for (i = 0; i < k; i++) {
    switch(format[i]) {
      case 'i':
        value[i].i = *((int *)swap_4bytes(&msg[j], swap));
        printf("int[%d]=%d\n", i ,value[i].i);
        j += 4;
        break;
      case 'f':
        value[i].f = *((float *)swap_4bytes(&msg[j], swap));
        printf("flt[%d]=%f\n", i ,value[i].f);
        j += 4;
        break;
      case 's':
        value[i].s = &msg[j];
        j = round_up_4bytes(strlen(&msg[j]) + 1);
        break;
      case 'T':
        value[i].b = 1;
        break;
      case 'F':
        value[i].b = 0;
        break;
      default:
        // b (blob), N (null), I (impulse), t (timetag)
        printf("osc: unhandled format id '%c'", format[i]);
        break;
    }
  }
  hexdump(msg, j);
  return;
Error:
  printf("osc message: pos=%d\n", j);
  hexdump(msg, 512);
  return;
}

void main(int argc, char **argv) {
  char buf[512];
  int len;

  while(!feof(stdin)) {
    if (fread(buf, 1, 4, stdin) <= 0)
      break;

    len = *((int *)swap_4bytes(buf, swap));
    printf("osc msg len: %d\n", len);

    fread(buf, 1, len, stdin);
    parse(buf);

    printf("\n\n");
  }
}