# {varname}> allocates a descriptor from 10 up
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
exec {fd}>/tmp/f
echo "fd=$fd"
echo hello >&$fd
exec {fd2}>/tmp/g
echo "fd2=$fd2"
exec {fd}>&-
echo closed >&$fd
echo status $?
show /tmp/f
exec {in}</tmp/f
echo "in=$in"
read -u $in line
echo "line=$line"
exec {in}<&-
exec {fd2}>&-
echo temp {t}>/tmp/h
echo "t=$t"
{ echo grouped >&$t; } {t}>/tmp/k
show /tmp/k
exec {dup}>&1
echo "dup=$dup"
echo via-dup >&$dup
exec {dup}>&-
exec {nv}>&-
echo "status $?"
bad=abc
exec {bad}>&-
echo "status $?"
