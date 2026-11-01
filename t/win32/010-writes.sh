# Shell tasks writing to a program's pipe while it is full: helpers
# write for them, in order per writer, and nothing is lost
f() { i=0; while [ $i -lt 20000 ]; do echo "$1$i"; i=$((i+1)); done; }
{ f a & f b; wait; } | "$U" -c 'sleep 1; n=0; while read l; do n=$((n+1)); done; echo $n'
f '' | "$U" -c 'sleep 1; cat' | { n=0; s=0; while read l; do n=$((n+1)); s=$((s+l)); done; echo "$n $s"; }
# A write to a pipe whose reader is gone is SIGPIPE: status 141
f x | "$U" -c 'read l; echo "$l"'
echo "${PIPESTATUS[*]}"
