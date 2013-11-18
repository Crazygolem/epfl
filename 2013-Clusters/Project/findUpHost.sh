#! /bin/sh

printHost() {
    color="$1"
    type="$2"
    host="$3"
    outfile="$4"
    
    if [ -n "$username" ]; then
        host=$username"@"$host
    fi
    
    if [ $directoutput -eq 0 ]; then
        printf "%s\t\033[0;"${color}"m[%s]\033[m\n" "$host" "$type"
        echo $host >> "$outfile"
    else
        if [ $color -eq 32 ]; then
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

hostbase="icin2pc" # Hostname has the form 'ICBC07PCxx' (try with: 'icbc07pc', 'icin2pc')
upperlimit=80       # ~ Number of PCs (max 'xx' of the hostname)
mpiport=8676        # Default MPI port
directoutput=0      # Outputs linux hosts to screen instead of file
username=""

while [ $# -gt 0 ]
do
    case "$1" in
    -h)   hostbase=$2; shift ;;
    -u)   username=$2; shift ;;
    -l)   upperlimit=$2; shift ;;
    -d)   directoutput=1;;
    *)    echo "usage: $0 [-h host basename] [-u username] [-l limit] [-d]" >&2
          exit 1;;
    esac
shift
done


WINFILE="hosts_up_win.txt"
MPIFILE="hosts_up_winmpi.txt"
LINFILE="hosts_up_linux.txt"
OFFFILE="hosts_down.txt"

rm -f "$WINFILE" "$MPIFILE" "$LINFILE" "$OFFFILE"
touch "$WINFILE" "$MPIFILE" "$LINFILE" "$OFFFILE"


for i in `seq -f"%02.0f" $upperlimit` ; do
    pingHost ${hostbase}$i &
done

wait

if [ $directoutput -eq 0 ]; then
    printf "Linux hosts:\t %d\n" $( wc -l $LINFILE | awk '{print $1}' )
    printf "Windows hosts:\t %d\n" $( wc -l $WINFILE | awk '{print $1}' )
    printf "MPI hosts:\t %d\n" $( wc -l $MPIFILE | awk '{print $1}' )
    printf "Offline hosts:\t %d\n" $( wc -l $OFFFILE | awk '{print $1}' )
fi
