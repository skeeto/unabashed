# personality: native
# Hand-checked baseline: unlike busybox-w32, the native persona has
# /dev/clipboard as msys mode does (044, 045), the Windows clipboard's
# text through the shell's own redirections. A program given the name
# as an argument opens C:/dev/clipboard, which does not exist.
show() { printf '%s [' "$1"; clipctl get; printf '] '; clipctl count; }
[ -c /dev/clipboard ] && [ -r /dev/clipboard ] && [ -w /dev/clipboard ] && echo "a character device"
read -r x </dev/clipboard; echo "empty $? [$x]"
echo hi >/dev/clipboard; show echo
{ echo a; argv b; echo c; } >/dev/clipboard; show mixed
busybox echo bb >/dev/clipboard; show busybox
printf 'one\r\ntwo' | clipctl set
x=$(</dev/clipboard); printf %s "$x" | od -c
busybox cat </dev/clipboard | od -c
busybox cat /dev/clipboard; echo "argument $?"
