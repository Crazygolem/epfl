#! /bin/sh

# Debug
#set -xv


WINFILE="hosts_up_win.txt"
MPIFILE="hosts_up_winmpi.txt"
LINFILE="hosts_up_linux.txt"
OFFFILE="hosts_down.txt"

hostbase="icin2pc"  # Hostname has the form 'ICBC07PCxx' (try with: 'icbc07pc', 'icin2pc')
upperlimit=80       # ~ Number of PCs (max 'xx' of the hostname)
mpiport=8676        # Default MPI port
directoutput=0      # Outputs linux hosts to screen instead of file
filteroutput=0      # Filters output
mpionly=0           # Outpus only MPI hosts
linuxonly=0         # Outputs only linux hosts
username=""


usage() {
    cat <<EOF
    Usage: $0 [options]
Options
    -h: Prints this help and exits
    -s: Outputs hostnames to standard output instead of files
    -p <prefix>: Hostname prefix
        Hostnames have the form 'ICBC07PCxx'
        (Try with: 'icbc07pc', 'icin2pc')
    -l <n>: ~ Number of PCs (max 'xx' of the hostname)
    -u <username>: Username (should not be required)
    --mpi: Outputs only MPI hosts
    --linux: Outputs only linux hosts
EOF
}

# Output filtering
# Returns 0 if it must not be output, else 1
# Allows to combine several filters:
#   --mpi --linux = Outputs only MPI and linux hosts
useOutput() {
    outfile="$1"

    if [ $filteroutput -eq 0 ]; then
        echo 1
    elif [ "$outfile" = "$MPIFILE" ]; then
        if [ $mpionly -eq 1 ]; then
            echo 1
        else
            echo 0
        fi
    elif [ "$outfile" = "$LINFILE" ]; then
        if [ $linuxonly -eq 1 ]; then
            echo 1
        else
            echo 0
        fi
    else
        echo 0
    fi
}

printHost() {
    color="$1"
    type="$2"
    host="$3"
    outfile="$4"
    
    if [ -n "$username" ]; then
        host=$username"@"$host
    fi

    if [ $(useOutput "$outfile") -ne 0 ]; then
        if [ $directoutput -eq 0 ]; then
            printf "%s\t\033[0;"${color}"m[%s]\033[m\n" "$host" "$type"
            echo $host >> "$outfile"
        else
            echo $host
        fi
    fi
}

pingHost() {
    host=$1

    if ping -w 1 -c 1 $host > /dev/null 2> /dev/null; then
        if nc -w 2 -z $host 22 ; then
            printHost 32 "UP - Linux" "$host" "$LINFILE"
        else

            if nc -w 2 -z $host $mpiport ; then
              printHost 34 "UP - Windows + MPI" "$host" "$MPIFILE"
            else
              printHost 35 "UP - Windows" "$host" "$WINFILE"
            fi
        fi
    else
        printHost 31 "Offline" "$host" "$OFFFILE"
    fi
}

echoStats() {
    message="$1"
    file="$2"

    if [ -e "$file" ]; then
        printf "${message}:\t %d\n" $( wc -l "$file" | awk '{print $1}' )
    else
        printf "${message}:\t N/A\n"
    fi
}


# Args parsing
while [ $# -gt 0 ]
do
    case "$1" in
    -h) usage >&2; exit;;
    -p) hostbase=$2; shift ;;
    -u) username=$2; shift ;;
    -l) upperlimit=$2; shift ;;
    -s) directoutput=1;;
    --mpi)      filteroutput=1; mpionly=1;;
    --linux)    filteroutput=1; linuxonly=1;;
    *)  echo "Invalid argument: $1. Try $0 -h" >&2
        exit 1;;
    esac
shift
done

# Cleaning up old files
rm -f "$WINFILE" "$MPIFILE" "$LINFILE" "$OFFFILE"

# Finding hosts
for i in `seq -f"%02.0f" $upperlimit` ; do
    pingHost ${hostbase}$i &
done

# Printing stats
wait    # Wait before all hosts have been tested
if [ $directoutput -eq 0 ]; then
    echoStats "Linux hosts" "$LINFILE"
    echoStats "Windows hosts" "$WINFILE"
    echoStats "MPI hosts" "$MPIFILE"
    echoStats "Offline hosts" "$OFFFILE"
fi