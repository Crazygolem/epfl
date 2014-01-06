#!/usr/bin/sh

OUTFILE="speedups"

cat <<EOF >> "${OUTFILE}.tmp"
nr_nodes theoretical measured amdahl newmodel
1        0.230       0.331    1.000  0.333
2        0.260       0.610    2.000  0.508
4        0.278       0.893    3.999  0.713
6        0.284       0.987    5.999  0.840
8        0.287       1.039    7.997  0.930
10       0.289       1.098    9.996  0.999
12       0.290       1.099    11.993 1.054
EOF

# First speedup model
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

# New speedup model
gnuplot <<EOS
set terminal svg size 640,380
set size 1,1

set output '${OUTFILE}_newmodel.svg'

set multiplot layout 1,1 title "Speedups comparison" scale 1,0.8 offset 0,0.05

set xlabel "# CPUs"
set ylabel "Speedup"

set key at screen 0.5,screen 0.1 center center Left reverse maxcols 3 maxrows 1

set xrange [0:13]
set yrange [0.15:1.4]

plot '${OUTFILE}.tmp' using 1:2 title "First model" with linespoints linewidth 1, \
     '${OUTFILE}.tmp' using 1:3 title "Measured" with linespoints linewidth 1, \
     '${OUTFILE}.tmp' using 1:5 title "New model" with linespoints linewidth 1

unset multiplot
EOS

rm "${OUTFILE}.tmp"