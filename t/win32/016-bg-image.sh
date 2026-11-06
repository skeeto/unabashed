# Background jobs are processes of this shell started from its state
# image (M5b): $! is the job's real pid and its BASHPID, $$ the
# parent's, and the job sees the parent's whole state
x=outer
declare -a arr=(a "b c")
declare -A as=([k]=v)
g() { echo "g $*"; }
f() {
    local l=loc
    { echo "$BASHPID $$" >pids.txt; echo "job $x $l ${arr[1]} ${as[k]}"; g "$@"; } &
    wait $!
    echo "status $?"
    read -r bp dd <pids.txt
    [ "$bp" = "$!" ] && echo "BASHPID is \$!"
    [ "$dd" = "$$" ] && echo "\$\$ is the parent's"
}
f one two

# descriptors: a file above 2, a pipe in, a here-string, a pipe out
exec 3>fd3.txt
{ echo three >&3; } &
wait
exec 3>&-
cat fd3.txt
echo piped | { read -r l; echo "got $l"; } &
wait
{ cat; } <<<"here" &
wait
{ echo out; } & wait
v=$( { echo captured; } & wait )
echo "$v"

# signals end it as killed by them
sleep 5 &
kill $!
wait $!
echo "TERM $?"
( exit 7 ) &
wait $!
echo "exit $?"

# a job outlives the shell that started it, which does not wait for it
SECONDS=0
"$U" -c '(sleep 2; echo late >late.txt) >/dev/null 2>&1 & echo started'
[ $SECONDS -lt 2 ] && echo "returned at once"
[ -e late.txt ] || echo "not yet"
sleep 3
cat late.txt
