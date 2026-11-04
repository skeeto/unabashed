# The code an external pipeline stage's words run (funsubs, prompt
# strings), and command_not_found_handle, run alongside the stages to
# their right: each waits here on one of those or writes past a pipe
rm -f f1 f2
wait1() {
    local i
    for i in 1 2 3 4 5 6 7 8 9 10; do
        [ -e "$1" ] && break
        sleep 0.1
    done
    [ -e "$1" ] && echo "seen $1" >&2 || echo "unseen $1" >&2
}
true | cat ${ wait1 f1; } - | { : >f1; cat; }; echo "1 $?"
p='$(wait1 f2)'
true | cat ${p@P} - | { : >f2; cat; }; echo "2 $?"
rm -f f1 f2
command_not_found_handle() {
    echo "h $1" >&2
    local i
    for ((i = 0; i < 30000; i++)); do echo "$i"; done
}
true | PATH=/nowhere cat | wc -l; echo "3 $?"
true | cat ${a[PATH=0]} | wc -l; echo "4 $?"
echo a | nosuch | wc -l; echo "5 $?"
