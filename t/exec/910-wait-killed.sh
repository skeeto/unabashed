# A script reports a background job a signal killed (not INT, TERM or
# PIPE) when wait collects it, a bare wait too, even when it ended
# before the wait; one never collected goes unreported. With and
# without job control.
exec 2>&1
for m in +m -m; do
    for s in HUP KILL USR1; do
        bash -c "set $m; sleep 5 & kill -s $s \$!; wait; echo $m $s \$?"
        bash -c "set $m; sleep 5 & kill -s $s \$!; wait \$!; echo $m $s pid \$?"
        bash -c "set $m; sleep 5 & kill -s $s \$!; sleep 0.2; echo $m $s later"
    done
    bash -c "set $m; sleep 5 & kill -s TERM \$!; wait; echo $m TERM \$?"
done 2>&1 | sed 's/  *[0-9][0-9]* / PID /'
