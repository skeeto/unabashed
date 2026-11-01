v=0
l1() { local v=1; echo "l1 start $v"; l2; echo "l1 end $v"; }
l2() { echo "l2 sees $v"; local v=2; echo "l2 own $v"; l3; echo "l2 end $v"; }
l3() { echo "l3 sees $v"; v=3; echo "l3 set $v"; }
l1
echo "top $v"
a() { local v=a; b; }
b() { local v; echo "b: [${v-unset}]"; v=b; c; }
c() { echo "c: $v"; }
a
echo "top $v"
