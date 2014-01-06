usage() {
    cat <<EOF
Description
    Finds DPS hosts that don't behave correctly.
    Available hosts should be actual MPI hosts.

Usage
    $0 [options] [start [end]]

    start: line number of the first host to test [Default: 1]
    end: line number of the last host to test [Default: last host line]

Options
    -h: Prints this help and exits
    -f <hosts file>: Specify the hosts file
EOF
}

countlines() {
  cat "$1" | sed '/^$/d' | {
    i=0
    while read -r || [[ -n "$REPLY" ]]; do
      i=$(($i + 1))
    done
    echo $i
  }
}

# Args parsing
while [ $# -gt 0 ]
do
    case "$1" in
    --) # Start processing standard positional arguments
        shift; break;;
    -h) usage >&2; exit;;
    -f) MACHINEFILE="$2"; shift;;
    -*) # Other optional argument that starts with a dash
        echo "Invalid argument: $1. Try $0 -h" >&2
        exit 1;;
    *) # Start processing standard positional arguments
        break;;
    esac
  shift
done

MACHINEFILE=${MACHINEFILE:-"hosts_up_winmpi.txt"}
START_NHOSTS=${1:-1}
END_NHOSTS=${2:-$(countlines "$MACHINEFILE")}


# Testing hosts

echo "Testing hosts #$START_NHOSTS to #$END_NHOSTS"
echo

offset=0

for i in $(seq $START_NHOSTS $END_NHOSTS); do
  line=$(($i - $offset)) # When a host is removed, line number must be shift
  hostname=$(sed -n "${line}{p;q;}" "$MACHINEFILE")

  echo "Host #$i: $hostname"
  mpiexec -mapall -machinefile "$MACHINEFILE" -n $line Parallel -max 100 -slaves 1 > /dev/null
  EC="$?"
  if [ $EC -ne 0 ]; then
    echo "    FAILURE"
    echo "    Removing host $hostname"
    offset=$(($offset + 1))

    # Cannot use `sed -i` because of weird permissions bug with shared drive
    # http://superuser.com/questions/392596/cygwin-bash-sed-locks-my-files
    sed "${line}{d;q;}" "$MACHINEFILE" > "$MACHINEFILE.tmp"
    rm -f "$MACHINEFILE"
    mv "$MACHINEFILE.tmp" "$MACHINEFILE"
  else
    echo "    OK"
  fi
done