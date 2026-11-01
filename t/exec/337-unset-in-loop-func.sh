count=0
f() { local c=$count; count=$((count + 1)); echo "call $c"; }
for i in 1 2 3; do f; done
echo "count=$count c=[${c-unset}]"
acc() { total=$((total + $1)); }
total=0
for v in 4 5 6; do acc $v; done
echo "total=$total"
while [ $count -gt 0 ]; do unset -v c; count=$((count - 1)); done
echo "count=$count"
