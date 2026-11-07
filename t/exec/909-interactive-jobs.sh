# An interactive shell names each job it starts in the background, "[N]
# PID" on standard error (not in a subshell or command substitution).
# With job control it reports an ended job, as jobs lists it, when wait
# collects it, between the commands of a `;` list (not in POSIX mode),
# and before each prompt, then drops it; without job control, it drops
# ended jobs unreported. A wait with no operands forgets every status.
# $- lists i after h. (Lines typed begin with a space, so that bash's
# echo of them on a non-terminal can be removed.)
exec 2>&1
run() {
    printf '%s\n' "$@" >cmds
    PS1= bash -i <cmds 2>&1 | sed '/^ /d; /tcsetattr/d; s/\] [0-9][0-9]*$/] PID/; s/group ([0-9-]*)/group (N)/; s/pid [0-9]* is/pid N is/'
}
run ' set -m; echo $-' \
    ' sleep 0.1 & wait; sleep 0.1 & wait' \
    ' true & sleep 0.1; false; exit 3 & wait $!; echo st=$?' \
    ' sleep 5 &' ' kill %1; wait' \
    ' sleep 5 &' ' kill -9 %1; sleep 0.1' ' echo next' \
    ' sleep 0.1 & sleep 0.2 & wait %2; echo w=$?' ' sleep 0.3' \
    ' sleep 0.1 & sleep 0.2 &' ' wait -n; echo n=$?' ' jobs' ' sleep 0.3' \
    ' false & wait; wait $!; echo last=$?' \
    ' set -o posix; sleep 0.1 & wait; echo posix' ' :'
echo ---
run ' echo $-; x=$(sleep 0 &); (sleep 0 &); coproc cat </dev/null' \
    ' wait; sleep 0.1 & wait; sleep 0.1 & wait' \
    ' true & sleep 0.1; wait; jobs' \
    ' sleep 0.1 & sleep 0.2' ' jobs; echo end'
