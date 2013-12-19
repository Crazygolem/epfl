OUTFILE="speedups"

cat <<EOF >> "${OUTFILE}.tmp"
nr_nodes theoretical measured
1 0.00002515 0.33
2 0.00002514 0.61
4 0.00002512 0.89
6 0.00002510 0.99
8 0.00002508 1.04
10 0.00002506 1.10
12 0.00002504 1.10
EOF

gnuplot <<EOS
set terminal png size 640,320
set size 1,1

set output '${OUTFILE}.png' 

set yrange [-0.1:1.2]

set title "Theoretical vs measured speedup"
set xlabel "# CPUs"
set ylabel "Speedup"
set key left


plot '${OUTFILE}.tmp' using 1:2 title "Theoretical" with linespoints linewidth 2, \
     '${OUTFILE}.tmp' using 1:3 title "Measured" with linespoints linewidth 2
EOS

rm "${OUTFILE}.tmp"