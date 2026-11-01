# duplication targets from expansions
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
exec 3>/tmp/f
n=3
echo via-var >&$n
exec 3>&-
show /tmp/f
e=
echo x >&$e
echo status $?
m="1 2"
echo y >&$m
echo status $?
echo z 2>&foo
echo status $?
read q <&bar
echo status $?
dash=-
echo closed >&$dash
echo status $?
echo w >&"$n"x
echo status $?
c=7
echo p >&$c
echo status $?
echo q 2>&$c
echo status $?
read r <&$c
echo status $?
read r 3<&$c
echo status $?
echo s >&$c-
echo status $?
