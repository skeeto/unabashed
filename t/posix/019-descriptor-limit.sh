# Under a low ulimit -n a command either runs as it would, or fails
# where everyone sees it: a message, "Too many open files", and a
# failing status. Never silently, as when a pipeline stage could not
# copy its descriptors and ran without them, its output or messages
# lost and its status 0. Where in the range each shell needs more
# descriptors than it has differs, so each limit prints only whether
# the run was one or the other.
cases='echo hi | cat
{ echo out; echo err >&2; } 2>&1 | cat
echo hi | cat | cat
printf "a\nb\n" | while read -r x; do echo "<$x>"; done | cat
echo hi | { read -r x; echo "$x"; } | cat'
st='; echo "st $? ${PIPESTATUS[*]}"'
printf '%s\n' "$cases" | while IFS= read -r c; do
    echo "## $c"
    full=$(bash -c "$c$st" 2>&1)
    for n in 4 5 6 7 8 9 10 11 12 13 14 16 20 24; do
        out=$(bash -c "ulimit -n $n; $c$st" 2>&1)
        shown=$(printf %s "$out" | tr '\n' '|')
        if [ "$out" = "$full" ]; then
            r=ok
        elif ! printf '%s\n' "$out" | grep -q 'Too many open files'; then
            r="LOST: $shown"
        elif printf '%s\n' "$out" | grep -q '^st [0 ]*$'; then
            r="LOST status: $shown"
        else
            r=ok
        fi
        echo "$n $r"
    done
done
