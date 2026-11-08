# Shell tasks writing to a program's pipe while it is full: helpers
# write for them, in order per writer, and nothing is lost
f() { i=0; while [ $i -lt 20000 ]; do echo "$1$i"; i=$((i+1)); done; }
{ f a & f b; wait; } | "$U" -c 'sleep 1; n=0; while read l; do n=$((n+1)); done; echo $n'
f '' | "$U" -c 'sleep 1; cat' | { n=0; s=0; while read l; do n=$((n+1)); s=$((s+l)); done; echo "$n $s"; }
# A write to a pipe whose reader is gone is SIGPIPE: status 141. The
# writer goes on until then: a fixed amount (120 KB) at times all fit,
# the pipe's quota coming back although nothing read it (issues.md)
g() { i=0; while :; do echo "x$i"; i=$((i+1)); done; }
g | "$U" -c 'read l; echo "$l"'
echo "${PIPESTATUS[*]}"
