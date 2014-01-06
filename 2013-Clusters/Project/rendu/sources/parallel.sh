MACHINEFILE='hosts_up_winmpi.txt'
OUTFILE='result_parallel.dat'

TRIALS=5
MAX=800000000
SLAVES="1 2 4 6 8 10 12"

rm -f "$OUTFILE"

echo "range_start range_end nr_slaves primes_count wall_time_ms" >> "$OUTFILE"
for slaves in $SLAVES; do
  echo "Slaves: $slaves"
  for trial in $(seq $TRIALS); do
    echo "  Trial $trial / $TRIALS"
    #set -xv
    mpiexec -mapall -machinefile "$MACHINEFILE" -n $(($slaves + 1)) Parallel -slaves $slaves -max $MAX -verbosity quiet \
    | tail -n +3 \
    >> "$OUTFILE"
    sleep 2
  done
done