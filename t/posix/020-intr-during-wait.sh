# An INT sent to a script (kill) while it waits. Untrapped, it ends a
# read, mapfile or select at once, as bash's read is interrupted, not
# once input comes; a pipeline or subshell the script waits for runs
# on, as bash's child, which the kill did not reach, and the script
# acts on the INT only if the child died of it. Trapped, the trap runs
# at once during a read, mapfile or select, which then reads on, and
# after the child for a pipeline or subshell, even one running in the
# shell's own process. The script runs nothing else meanwhile; the
# kill and its input, which comes late, are this one's.
pf=/tmp/020-pid.$$
cases='read x; echo "read $? $x"
read -n 3 x; echo "read -n $? $x"
read -t 5 x; echo "read -t $? $x"
mapfile -t m; echo "mapfile $? ${m[*]}"
select v in a b; do echo "v$v"; break; done; echo "select $?"
sleep 1 | cat; echo "pipeline $?"
(sleep 1); echo "subshell $?"
{ sleep 1; echo ab; } | { read x; echo "r$x"; }; echo "stage $?"
x=$(sleep 1; echo ab); echo "comsub $? $x"'
printf '%s\n' "$cases" | while IFS= read -r c; do
    echo "## $c"
    for trap in "" "trap 'echo T' INT; "; do
        : >"$pf"
        { sleep 0.3; read -r pid <"$pf"; kill -INT "$pid"; } &
        start=${EPOCHREALTIME/./}
        bash -c "echo \$\$ >$pf; $trap$c" 2>&1 < <(sleep 1.5; echo 1)
        st=$?
        end=${EPOCHREALTIME/./}
        wait
        if [ $((end - start)) -lt 1200000 ]; then t=soon; else t=late; fi
        echo "status $st $t"
    done
done
