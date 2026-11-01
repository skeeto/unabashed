# redirections attached to compound commands
for i in 1 2 3; do echo "line $i"; done > /tmp/f
while read l; do echo "got $l"; done < /tmp/f
n=0
while read l; do n=$((n+1)); done < /tmp/f
echo "count $n"
if true; then echo "if out"; fi > /tmp/g
case x in x) echo "case out";; esac >> /tmp/g
{ echo grp; } >> /tmp/g
until true; do :; done >> /tmp/g
for ((i=0; i<1; i++)); do echo "arith out"; done >> /tmp/g
while read l; do echo "g: $l"; done < /tmp/g
for i in 1; do echo bad; done > /nonexistent/x
echo "bad redirect: $?"
while read l; do echo never; done < /tmp/missing
echo "missing input: $?"
if echo "never"; then :; fi > /nonexistent/y
echo "if bad redirect: $?"
while read l
do
    echo never
done < /tmp/missing2
echo "multi-line: $?"
{
    echo never
} > /nonexistent/z
if true
then
    echo never
fi 2>/dev/null < /tmp/missing3
echo "if multi: $?"
