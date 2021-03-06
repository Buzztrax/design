#!/bin/bash

gcc -Wall -g dynlink3.c -o dynlink3 `pkg-config gstreamer-1.0 --cflags --libs`
if test $? -ne 0; then
  exit 1
fi
rm dyn*.{dot,png}

trap "echo ' aborted ...'" SIGINT
GST_DEBUG="dynlink:2" GST_DEBUG_DUMP_DOT_DIR=$PWD ./dynlink3

for file in dyn*.dot; do echo $file; dot -Tpng $file -o${file/dot/png}; done
eog *.png

