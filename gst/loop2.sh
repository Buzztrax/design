#!/bin/bash

LOG="loop2.log"

# get latency time:
grep -o "changing audio chunk-size for sink.*" "${LOG}"

# print time the sources are paused
echo "time the sources are stopped in µs"
ts_list=$(egrep "(pausing after end|loop playback)" "${LOG}" | cut -d' ' -f1 | cut -d':' -f3)
while read -r l1 && read -r l2; do
  printf '  %7.5f µs\n' $(echo "($l2 - $l1) * 1000" | bc);
done < <(echo "$ts_list")

echo "events"
grep -o -i "discont" "${LOG}" | sort | uniq -c
grep -o -i "gap" "${LOG}" | sort | uniq -c

#
# egrep -i "(segment_done|discont|gap)" loop2.log

# plot media time (y) vs. when data was received = log-ts (x)
# FIXME: we expect a sawtooth/triangle  like curve
# 
grep "audiobasesink" loop2.log | grep "time 0" | sed -e 's/  */:/g' -e 's/,/:/g' | cut -d':' -f3,15 | sed -e 's/:0/ /' -e 's/^0//g' >/tmp/loop2_times.csv

cat | gnuplot <<EOF
set term png truecolor size 1024,768
set output 'loop2.png'
plot '/tmp/loop2_times.csv' using 1:2 with linespoints title 'sink'
EOF


