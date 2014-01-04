#!/usr/bin/sh

OUTFILE="speedups"

cat <<EOF >> "${OUTFILE}.tmp"
nr_nodes theoretical measured amdahl
1 0.230 0.33 1.00
2 0.260 0.61 2.00
4 0.278 0.89 4.00
6 0.284 0.99 6.00
8 0.287 1.04 8.00
10 0.289 1.10 10.00
12 0.290 1.10 11.99
EOF

gnuplot <<EOS
set terminal svg size 640,380
set size 1,1

set output '${OUTFILE}.svg'

set multiplot layout 1,2 title "Speedups comparison" scale 1,0.8 offset 0,0.05

set xrange [0:13]
set xlabel "# CPUs"
set ylabel "Speedup"

# Plot 1: Without Amdahl's speedup
unset key # Subset of second plot's key, print only once
set yrange [0.15:1.2]
plot '${OUTFILE}.tmp' using 1:2 title "Theoretical" with linespoints linewidth 1, \
     '${OUTFILE}.tmp' using 1:3 title "Measured" with linespoints linewidth 1


# Plot 2: With Amdahl's speedup
set key at screen 0.5,screen 0.1 center center Left reverse maxcols 3 maxrows 1
set yrange [-1:13]
plot '${OUTFILE}.tmp' using 1:2 title "Theoretical" with linespoints linewidth 1, \
     '${OUTFILE}.tmp' using 1:3 title "Measured" with linespoints linewidth 1, \
     '${OUTFILE}.tmp' using 1:4 title "Amdahl's" with linespoints linewidth 1

unset multiplot
EOS

rm "${OUTFILE}.tmp"