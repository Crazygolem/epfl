MACHINEFILE='hosts_up_winmpi.txt'
OUTFILE='result.dat'

TRIALS=5
MINMAX=12500000
MAXMAX=3200000000

rm $OUTFILE

echo "range_start range_end primes_count wall_time_ms" >> "$OUTFILE"
for i in $(seq $TRIALS); do
  max=$MINMAX
  while [ $max -le $MAXMAX ]; do
    echo "Sieving up to $max"
    ./Serial $max quiet >> "$OUTFILE"
    max=$(($max * 2))
  done
done