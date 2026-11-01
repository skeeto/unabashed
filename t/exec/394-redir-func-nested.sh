# nested redirections in functions restore on return
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
inner() { echo "inner out"; echo "inner err" >&2; }
outer() {
    inner >/tmp/in 2>&1
    echo "outer after inner"
}
outer >/tmp/out
echo "-- in"; show /tmp/in
echo "-- out"; show /tmp/out
chain() { echo "chain $1"; } >/tmp/c
chain 1 >/tmp/d
echo "-- c"; show /tmp/c
echo "-- d"; read l </tmp/d; echo "d status $?"
loud() { echo "loud"; }
{ loud; loud >&2; } 2>/tmp/e >/dev/null
show /tmp/e
