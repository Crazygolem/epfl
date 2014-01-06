gnuplot -e 'pmax=10;Lmax=100' achievable-speedup.plot
gnuplot -e 'pmax=10;Lmax=1000' achievable-speedup.plot
gnuplot -e 'pmax=10;Lmax=100000' achievable-speedup.plot
gnuplot -e 'pmax=100;Lmax=100' achievable-speedup.plot
gnuplot -e 'pmax=100;Lmax=10000' achievable-speedup.plot
gnuplot -e 'pmax=1000;Lmax=1000000' achievable-speedup.plot
gnuplot -e 'pmax=1000;Lmax=100' achievable-speedup.plot
gnuplot -e 'pmax=100000;Lmax=100' achievable-speedup.plot
gnuplot -e 'pmax=100000;Lmax=100000' achievable-speedup.plot

for f in *.svg; do
  convert $f ${f%.*}.png
done