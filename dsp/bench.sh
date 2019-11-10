#!/bin/sh
#
# See https://easyperf.net/blog/2019/08/02/Perf-measurement-environment-on-Linux

# performance, powersave
mode=$1

for i in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
do
  echo $mode > $i
done
