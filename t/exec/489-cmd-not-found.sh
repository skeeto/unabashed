nosuch
echo "st=$?"
nosuch arg1 arg2
echo "st=$?"
"no such"
echo "st=$?"
''
echo "st=$?"
/no/such/path
echo "st=$?"
./nosuch
echo "st=$?"
x=$(nosuch2)
echo "sub st=$?"
nosuch3 2>/dev/null
echo "quiet st=$?"
nosuch4 | true
echo "pipe st=$?"
true | nosuch5
echo "pipe2 st=$?"
v=nosuch6; $v
echo "var st=$?"
empty=; $empty
echo "empty st=$?"
