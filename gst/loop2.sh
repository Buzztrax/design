#!/bin/bash

LOG="debug.log"

# get latency time:
grep -o "changing audio chunk-size for sink.*" "${LOG}"

# print time the sources are paused
echo "time the sources are stopped in µs"
ts_list=$(egrep "(pausing after end|loop playback)" "${LOG}" | cut -d' ' -f1 | cut -d':' -f3)
while read -r l1 && read -r l2; do
  printf '  %7.5f µs\n' $(echo "($l2 - $l1) * 1000" | bc);
done < <(echo "$ts_list")

