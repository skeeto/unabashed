# personality: msys
# Hand-checked against Git Bash 5.3.15 (notes/research/msys2.md): what
# is written through one open of /dev/clipboard accumulates, each of the
# shell's writes setting the clipboard to all of it; >> starts empty too,
# and an open without a write leaves the clipboard alone. Unlike Git
# Bash, where a child has its own buffer (its text lost to the parent's
# next write), a program's output joins the rest, set as it closes, and
# a busy clipboard is an error.
show() { printf '%s [' "$1"; clipctl get; printf '] '; clipctl count; }
echo hi >/dev/clipboard; show echo
{ printf a; printf b; } >/dev/clipboard; show group
: >/dev/clipboard; show "no write"
echo x >>/dev/clipboard; show append
exec 3>/dev/clipboard
echo one >&3
echo two >&3
exec 3>&-; show "exec 3>"
argv p q >/dev/clipboard; show program
busybox true >/dev/clipboard; show "silent program"
{ echo a; argv b; echo c; } >/dev/clipboard; show mixed
echo in | busybox cat >/dev/clipboard; show "last stage"
{ busybox echo s | busybox cat; echo t; } >/dev/clipboard; show "pipeline in group"
( echo u; argv v ) >/dev/clipboard; show subshell
{ argv bg & } >/dev/clipboard; wait; show background
{ echo w & wait; echo y; } >/dev/clipboard; show "background vproc"
set -C; echo n >/dev/clipboard; echo "noclobber $?"; set +C; show noclobber
[ -w /dev/clipboard ] && [ -c /dev/clipboard ] && echo writable
exec 4>/dev/clipboard; [ -t 4 ] || [ -c /dev/fd/4 ] && echo "fd 4 a device"; exec 4>&-
printf 'caf\303\251' >/dev/clipboard; clipctl get | od -c
printf 'a\0b' >/dev/clipboard; clipctl get | od -c
clipctl busy 1
echo z >/dev/clipboard; echo "busy $?"; show busy
clipctl busy 1
argv zz >/dev/clipboard; echo "busy program $?"; show "busy program"
