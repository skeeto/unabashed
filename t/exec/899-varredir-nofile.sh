# {var} redirections need a descriptor from 10 up, which ulimit -n
# below 10 puts out of reach
exec {v}</dev/null; echo "v=$v"; exec {v}<&-
( ulimit -n 6; exec {v}</dev/null; echo "status $? ${v-unset}" )
( ulimit -n 6; exec 3</dev/null; echo "status $?"; exec {w}>&2; echo "${w-unset}" )
( ulimit -n 11; exec {v}</dev/null; echo "status $? ${v-unset}" )
