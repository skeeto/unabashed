echo hi > /nonexistent/f
echo "a st=$?"
echo hi < /nonexistent/in
echo "b st=$?"
: > /tmp
echo "c st=$?"
{ echo inner; } > /nonexistent/g
echo "d st=$?"
f() { echo infunc; }
f > /nonexistent/h
echo "e st=$?"
x=1 > /nonexistent/i
echo "f st=$? x=$x"
echo ok >&9
echo "g st=$?"
set -C
echo one > /tmp/clob
echo two > /tmp/clob
echo "h st=$?"
echo three >| /tmp/clob
echo "i st=$?"
read l < /tmp/clob; echo "l=$l"
for i in 1; do echo loop; done > /nonexistent/j
echo "j st=$?"
echo end
