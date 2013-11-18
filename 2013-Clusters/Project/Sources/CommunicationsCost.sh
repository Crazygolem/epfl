MACHINEFILE='hosts_up_winmpi.txt'
NR_MACHINES=11
OUTFILE='result.dat'
MAXPAYLOAD=7
TRIALS=3

rm $OUTFILE

for p in $(seq 1 $MAXPAYLOAD); do
  pp=$((10 ** $p))
  echo "Payload: $pp"
  for n in $(seq 1 $TRIALS); do
    echo "  Trial: $n / $TRIALS"
    mpiexec -mapall -machinefile hosts_up_winmpi.txt -n 11 Test -pat "$(($NR_MACHINES - 1))+1" -messages 10 -payload $pp | \
      head -n 13 | \
      tail -n +4 >> "${OUTFILE}"
    sleep 2
  done
done