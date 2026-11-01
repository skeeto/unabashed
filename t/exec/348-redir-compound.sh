# redirections on compound commands
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
{ echo g1; echo g2; } >/tmp/f
show /tmp/f
( echo s1; echo s2 ) >/tmp/f
show /tmp/f
if true; then echo i1; fi >/tmp/f
show /tmp/f
for i in 1 2 3; do echo f$i; done >/tmp/f
show /tmp/f
case x in x) echo c1;; esac >/tmp/f
show /tmp/f
n=0; while [ $n -lt 2 ]; do echo w$n; n=$((n+1)); done >/tmp/f
show /tmp/f
until true; do :; done >/tmp/f
show /tmp/f
echo empty-ok
[[ a == a ]] >/tmp/f && echo cond
(( 1 )) >/tmp/f && echo arith
{ echo x; } >/tmp/nodir/f
echo status $?
{
    echo multi
} >/tmp/nodir/g
echo status $?
while false
do :
done </tmp/nosuch
echo status $?
fr() {
    echo fr
} >/tmp/nodir/h
echo between
fr
echo status $?
for i in 1; do
    :
done >/tmp/nodir/i
echo status $?
