MACHINEFILE='hosts_up_winmpi.txt'
OUTFILE='communications_specific.dat'

noheader=""

starttest() {
  nrSlaves="$1"
  payloadSize="$2"
  nrTrials="$3"

  echo "Payload: $payloadSize"
  echo "Nr messages: $nrSlaves"

  for n in $(seq 1 $nrTrials); do
    echo "  Trial: $n / $nrTrials"
    mpiexec -mapall -machinefile $MACHINEFILE -n $(( $nrSlaves + 1 )) Test -slaves $nrSlaves -payload $payloadSize $noheader | \
      tail -n +3 | \
      head -n $(( $nrSlaves + 3 )) >> "$OUTFILE" # `head` to avoid long error messages
      
    # After first execution, verbosity set to "quiet" to avoid printing the header line
    noheader="-noheader"
    sleep 2
  done
}

rm -f $OUTFILE
starttest 1 0 20
starttest 1 80000000 10
starttest 2 40000000 10
starttest 4 20000000 10
starttest 6 13333333 10
starttest 8 10000000 10
starttest 10 8000000 10
starttest 12 6666666 10