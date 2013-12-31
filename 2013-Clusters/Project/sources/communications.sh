MACHINEFILE='hosts_up_winmpi.txt'
NR_MACHINES=11
OUTFILE='result.dat'
MAXPAYLOAD_P=7
TRIALS=10

rm -f $OUTFILE

echo "payload_size heavy_processing message_id rtt_ms" >> "$OUTFILE"
for p2 in 2; do
  pp=$(((10 ** $MAXPAYLOAD_P) * $p2))
  echo "Payload: $pp"
  for n in $(seq 1 $TRIALS); do
    echo "  Trial: $n / $TRIALS"
    mpiexec -mapall -machinefile hosts_up_winmpi.txt -n 11 Test -pat "$(($NR_MACHINES - 1))+1" -messages 10 -payload $pp | \
      head -n 13 | \
      tail -n +4 >> "$OUTFILE"
    sleep 2
  done
done