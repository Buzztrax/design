#!/bin/bash
#
# ./filter.sh <datafile> | gnuplot
#

data=$1
base=$(basename $data ".dat")

cat <<EOF
set term png truecolor font "Sans,5" size 1280,768
set output '$base.png'

set xtics nomirror autofreq
set ytics nomirror autofreq
set ylabel "frequency"
set ylabel "volume"
set logscale x
set key bottom
set grid
set key box

plot \\
  '$data' using 1:2 with lines title "slope 1", \\
  '' using 1:3 with lines title "slope 2", \\
  '' using 1:4 with lines title "slope 3"
EOF
