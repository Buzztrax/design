#!/bin/bash

export GST_DEBUG="latency:4"
export GST_DEBUG_NO_COLOR=1

bpm=125
tpb=4

test="alsasink_def"
echo "[$test] start"
./latency $bpm $tpb 1 "alsasink" 2>latencies/${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="pulsesink_push"
echo "[$test] start"
./latency $bpm $tpb 1 "pulsesink" 2>latencies/${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="pulsesink_push_2"
echo "[$test] start"
./latency $bpm $tpb 2 "pulsesink" 2>latencies/${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="pulsesink_push_4"
echo "[$test] start"
./latency $bpm $tpb 4 "pulsesink" 2>latencies/${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="pulsesink_pull"
echo "[$test] start"
./latency $bpm $tpb 1 "pulsesink can-activate-pull=true" 2>latencies/${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

# needs pasuspender to work

test="alsasink_hw"
echo "[$test] start"
pasuspender -- ./latency $bpm $tpb 1 "alsasink device=hw:0" 2>latencies/${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="alsasink_plughw"
echo "[$test] start"
pasuspender -- ./latency $bpm $tpb 1 "alsasink device=plughw:0" 2>latencies/${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="jacksink_push"
echo "[$test] start"
pasuspender -- jackd >/dev/null --silent --temporary -d alsa &
sleep 1s
./latency $bpm $tpb 1 "jackaudiosink" 2>latencies/${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="jacksink_pull"
echo "[$test] start"
pasuspender -- jackd >/dev/null --silent --temporary -d alsa &
sleep 1s
./latency $bpm $tpb 1 "jackaudiosink can-activate-pull=true" 2>latencies/${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

