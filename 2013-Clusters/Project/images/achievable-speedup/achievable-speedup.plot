if (!exists("pmax")) pmax=100
if (!exists("Lmax")) Lmax=10000

sp(x, y) = x/(1+(x-1)*(2/sqrt(y)))

set termopt enhanced    # Turn on enhanced text mode

set title "Achievable speedup"
set xlabel "p"
set ylabel "L"
set zlabel "S_{p}"

# Output
set terminal qt

unset key               # Remove plot formula
set view 45,315         # Orientation of the plot

set palette gray
set contour base
unset clabel            # All contour have the same color
set cntrparam levels 20 # Number of contour lines

set xrange [1:pmax]
set yrange [1:Lmax]

set multiplot

set isosamples 100      # High definition
splot sp(x, y) with pm3d

unset contour
set isosamples 10       # Low definition (less lines)
splot sp(x, y) with lines lc rgb "#ffffff"

unset multiplot

pause -1