# P4 (POSIX): command substitution of builtins and functions
f() { echo "v$1"; }
i=0; while [ $i -lt 10000 ]; do x=$(echo $i); i=$((i+1)); done; echo $x
i=0; while [ $i -lt 10000 ]; do x=$(f $i); i=$((i+1)); done; echo $x
i=0; while [ $i -lt 10000 ]; do x=`printf '%d' $i`; i=$((i+1)); done; echo $x
