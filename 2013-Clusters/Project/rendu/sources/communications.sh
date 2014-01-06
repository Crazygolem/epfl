MACHINEFILE='hosts_up_winmpi.txt'
OUTFILE='communications_rtt.dat'

NR_MACHINES="11 5" # Must be >= 2 (one master and one slave)
NR_TRIALS=10

# Returns a list of payloads (which might be long)
getPayloadSizes() {
  # minMag=1
  # maxMag=7
  # also="0 8000000 15000000 20000000 40000000 80000000"
  also="80000000 40000000 20000000 15000000"

  # for p in $(seq $minMag $maxMag); do
  #   echo $((10 ** $p))
  #   echo $(((10 ** $p) * 5))
  # done

  for p in $also; do
    echo $p
  done
}

starttest() {
  args="$1"
  noheader=""

  for delay in "none" "constant" "random" "distributed"; do
    echo "Load management delay: $delay"

    for payloadSize in $(getPayloadSizes); do
      echo "  Payload size: $payloadSize"

      for nrMachines in $NR_MACHINES; do
        nrSlaves=$(($nrMachines - 1))
        echo -n "    Number of slaves: $nrSlaves"
        
        # Memory limitations require to use different parameters
        tNrMachines=$nrMachines
        if [ $payloadSize -gt 40000000 ]; then
          tNrMachines=2 # 1 master, 1 slave
        elif [ $payloadSize -ge 20000000 ]; then
          tNrMachines=5
        fi

        if [ $nrMachines -gt $tNrMachines ]; then
          nrSlaves=$(($tNrMachines - 1))
          echo " -> $nrSlaves"
        else
          echo
        fi

        for n in $(seq 1 $NR_TRIALS); do
          echo "      Trial: $n / $NR_TRIALS"
          mpiexec -mapall -machinefile $MACHINEFILE -n $nrMachines Test -slaves $nrSlaves -payload $payloadSize -delay $delay $noheader $args | \
            tail -n +3 | \
            head -n $(( $nrSlaves + 3 )) >> "$OUTFILE" # `head` to avoid long error messages
            
          # After first execution, verbosity set to "quiet" to avoid printing the header line
          noheader="-noheader"
          sleep 2
        done
      done
    done
  done
}

rm -f $OUTFILE
starttest