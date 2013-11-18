MACHINEFILE='hosts_up_winmpi.txt'
NR_MACHINES=11
OUTFILE='result.dat'
MAXPAYLOAD_P=7
TRIALS=3

rm $OUTFILE

echo "payload_size heavy_processing message_id rtt_ms" >> "$OUTFILE"
for p1 in $(seq 1 $MAXPAYLOAD_P); do
  for p2 in 1 5; do
    pp=$(((10 ** $p1) * $p2))
    echo "Payload: $pp"
    for n in $(seq 1 $TRIALS); do
      echo "  Trial: $n / $TRIALS"
      mpiexec -mapall -machinefile hosts_up_winmpi.txt -n 11 Test -pat "$(($NR_MACHINES - 1))+1" -messages 10 -payload $pp | \
        head -n 13 | \
        tail -n +4 >> "$OUTFILE"
      sleep 2
    done
  done
done