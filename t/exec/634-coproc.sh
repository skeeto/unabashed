# Coprocesses: the unnamed and named forms, their descriptors and
# variables, data both ways, closing their ends, and a second one
coproc { read x; echo "got $x"; }
case $COPROC_PID in [0-9]*) echo "pid ok";; esac
echo ${COPROC[@]}
echo hello >&${COPROC[1]}
read line <&${COPROC[0]}
echo "$line"
wait $COPROC_PID; echo "status $?"
echo "after: ${COPROC[@]-unset} ${COPROC_PID-unset}"
coproc UP { while read l; do echo "${l^^}"; done; }
echo ${UP[@]}
echo abc >&${UP[1]}
read r <&${UP[0]}; echo "$r"
echo def >&${UP[1]}
read r <&${UP[0]}; echo "$r"
exec {UP[1]}>&-
echo "closed: ${UP[@]}"
wait $UP_PID; echo "up $?"
coproc CAT { cat; }
coproc X { sleep 1; }
echo ${X[@]}
wait $X_PID
coproc cat
echo one >&${COPROC[1]}
read r <&${COPROC[0]}; echo "$r"
exec 4<&${COPROC[0]}-
echo "moved: ${COPROC[@]}"
coproc 1x { :; }; echo "bad $?"
