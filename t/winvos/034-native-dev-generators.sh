# personality: native
# Hand-checked baseline: the native persona has /dev/zero and
# /dev/urandom only, as busybox-w32 does, read through a generator
# pump by builtins and programs, and written as sinks.
hex() { od -An -tx1 | tr -d ' \n'; echo; }
head -c 8 </dev/zero | hex
head -c 100000 </dev/zero | wc -c
exec 3</dev/urandom
a=$(head -c 16 <&3 | hex); b=$(head -c 16 <&3 | hex)
echo "${#a} ${#b}"
[ "$a" != "$b" ] && echo "one stream, moving on"
c=$( (head -c 16 </dev/urandom | hex) ); d=$(head -c 16 </dev/urandom | hex)
[ "$c" != "$d" ] && echo "a subshell does not replay"
exec 3<&-
IFS= read -rN 4 x </dev/urandom; echo "read $?"
[ -c /dev/zero ] && [ -c /dev/urandom ] && echo "character devices"
[ -e /dev/full ] || [ -e /dev/random ] || echo "no full or random"
echo hi >/dev/zero; echo "zero $?"
echo hi >/dev/urandom; echo "urandom $?"
