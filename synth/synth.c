//#!/usr/bin/tcc -run
// gcc synth.c -o synth
// ./synth | pacat --format=u8 --rate=8000--channels=1
//
// tcc -run synth.c | pacat --format=u8 --rate=8000--channels=1
//
// https://www.youtube.com/watch?v=GtQdIYUtAHg
//
// http://wurstcaptures.untergrund.net/music/
//
// formulas:
// http://pelulamu.net/countercomplex/music_formula_collection.txt

void main(int argc, char **argv)
{
  int t;
  
  for (t=0;;t++) {
    putchar(
      // saw @ 31.25 Hz
      //t
      // minimal sierpinski harmony
      //t&t>>8

      // tunes @ 8000 Hz
      // "the 42 melody", separately discovered by several people on irc etc
      //t*(42&t>>10)
      // danharaj 2011-10-03 http://www.reddit.com/r/programming/comments/kyj77/algorithmic_symphonies_from_one_line_of_code_how/ "fractal trees", 216's version
      //t|t%255|t%257
      // droid 2011-10-05 http://pouet.net/topic.php?which=8357&page=10
      //t>>6&1?t>>5:-t>>4
      // Niklas_Roy 2011-10-14 http://countercomplex.blogspot.com/2011/10/algorithmic-symphonies-from-one-line-of.html
      //t*(t>>9|t>>13)&16
      //(t>>13|t%24)&(t>>7|t%19)
      //(t*((t>>9|t>>13)&15))&129
      
      // krcko 2011-10-04 http://rafforum.rs/index.php/topic,123.0.html
      //(t&t>>12)*(t>>4|t>>8)

      //t*((t>>12|t>>8)&63&t>>4)
      //(t*(t>>5*t>>8))>>(t>>16)
      //t*((t>>9|t>>14)&25&t>>6)
      //t*(t>>11&t>>8&123&t>>3)
      //(t*(t>>8*(t>>15|t>>8))&(20|(t>>19)*5>>t|t>>3))
      //((-t&4095)*(255&t*(t&t>>13))>>12)+(127&t*(234&t>>8&t>>3)>>(3&t>>14))
      //t*(t>>((t>>9|t>>8))&63&t>>4)
      t*((t>>14|t>>9)&92&t>>5)

      //(t>>6|t|t>>(t>>16))*10+((t>>11)&7)
      //(t|(t>>9|t>>7))*t&(t>>11|t>>9)
      //t*5&(t>>7)|t*3&(t*4>>10)
      //(t>>7|t|t>>6)*10+4*(t&t>>13|t>>6)
      //((t&4096)?((t*(t^t%255)|(t>>4))>>1):(t>>3)|((t&8192)?t<<2:t))
      //((t*(t>>8|t>>9)&46&t>>8))^(t&t>>13|t>>6)
      
      //(t*5&t>>7)|(t*3&t>>10)
      //(int)(t/1e7*t*t+t)%127|t>>4|t>>5|t%127+(t>>16)|t
      //((t/2*(15&(0x234568a0>>(t>>8&28))))|t/2>>(t>>11)^t>>12)+(t/16&t&24)
      //(t&t%255)-(t*3&t>>13&t>>6)
      //(t*9&t>>4|t*5&t>>7|t*3&t/1024)-1
      //((t>>4)*(13&(0x8898a989>>(t>>11&30)))&255)+((((t>>9|(t>>2)|t>>8)*10+4*((t>>2)&t>>15|t>>8))&255)>>1)
      //(t%25-(t>>2|t*15|t%227)-t>>3)|((t>>5)&(t<<5)*1663|(t>>3)%1544)/((t%17|t%2048)|1)
      //((1-(((t+10)>>((t>>9)&((t>>14))))&(t>>4&-2)))*2)*(((t>>10)^((t+((t>>6)&127))>>10))&1)*32+128
      //t>>16|((t>>4)%16)|((t>>4)%192)|(t*t%64)|(t*t%96)|(t>>16)*(t|t>>5)
      //t>>6^t&37|t+(t^t>>11)-t*((t%24?2:6)&t>>11)^t<<1&(t&598?t>>4:t>>10) 
      //(t>>5)|(t>>4)|((t%42)*(t>>4)|(0x15483113)-(t>>4))/((t>>16)^(t|(t>>4))|1)
      //(t*t/256)&(t>>((t/1024)%16))^t%64*(0xC0D3DE4D69>>(t>>9&30)&t%32)*t>>18
      
      // tunes @ 44100 Hz
      //((t*("36364689"[t>>13&7]&15))/12&128)+(((((t>>12)^(t>>12)-2)%11*t)/4|t>>13)&127)
      //((t/2*(15&(0x234568a0>>(t>>8&28))))|t/2>>(t>>11)^t>>12)+(t/16&t&24)
    );
  }
}
