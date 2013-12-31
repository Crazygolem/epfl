MACHINEFILE='hosts_up_winmpi.txt'
NR_MACHINES=11 # Must be > 2 (one master and one slave)
OUTFILE='communications_rtt.dat'
NR_TRIALS=10

getPayloadSizes() {
  minMag=1
  maxMag=7
  also="0 80000000"

  for p in $(seq $minMag $maxMag); do
    echo $((10 ** $p))
    echo $(((10 ** $p) * 5))
  done

  for p in $also; do
    echo $p
  done
}

starttest() {
  args="$1"

  for payloadSize in $(getPayloadSizes); do
    trials=$NR_TRIALS
    nrMachines=$NR_MACHINES

    if [ $payloadSize -eq 0 ]; then
      magFactor="0"
    elif [ $payloadSize -gt 10000000 ]; then
      # Memory limitations require to use different parameters
      nrMachines=2 # 1 master, 1 slave
    fi

    nrSlaves=$(($nrMachines - 1))



    echo "  Payload: $payloadSize"
    for n in $(seq 1 $trials); do
      echo "    Trial: $n / $trials"
      mpiexec -mapall -machinefile $MACHINEFILE -n $nrMachines Test -pat "${nrSlaves}+1" -messages $nrSlaves -payload $payloadSize $args | \
        head -n 13 | \
        tail -n +4 >> "$OUTFILE"
      sleep 2
    done
  done
}

rm -f $OUTFILE

echo "payload_size slaves_processing message_id rtt_ms" >> "$OUTFILE"

echo "Slaves computations: None"
starttest
echo "Slaves computations: Constant"
starttest -slaves.constant
echo "Slaves computations: Random"
starttest -slaves.random
echo "Slaves computations: Distributed"
starttest -slaves.distributed