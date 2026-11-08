# personality: msys
# Hand-checked baseline: /dev/zero, /dev/full, /dev/random and
# /dev/urandom read through a generator pump, by builtins and by
# programs; random bytes by their properties only. Written, all are
# sinks but /dev/full, whose writes fail with ENOSPC.
hex() { od -An -tx1 | tr -d ' \n'; echo; }
head -c 8 </dev/zero | hex
head -c 8 </dev/full | hex
head -c 100000 </dev/zero | wc -c
exec 3</dev/urandom
a=$(head -c 16 <&3 | hex); b=$(head -c 16 <&3 | hex)
echo "${#a} ${#b}"
[ "$a" != "$b" ] && echo "one stream, moving on"
c=$( (head -c 16 </dev/urandom | hex) ); d=$(head -c 16 </dev/random | hex)
[ "$c" != "$d" ] && echo "a subshell does not replay"
e=$( (head -c 16 <&3 | hex) ); f=$(head -c 16 <&3 | hex)
[ "$e" != "$f" ] && echo "nor through a shared descriptor"
exec 3<&-
IFS= read -rN 4 x </dev/urandom; echo "read $?"
[ -c /dev/zero ] && [ -c /dev/urandom ] && echo "character devices"
echo hi >/dev/full; echo "full $?"
echo hi >/dev/zero; echo "zero $?"
echo hi >/dev/urandom; echo "urandom $?"
read -r y </dev/clipboard; echo "clipboard $?"
cat </dev/zero | head -c 4 | hex
