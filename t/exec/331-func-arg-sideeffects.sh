f() { echo "arg=[$1]"; }
f "${x=default}"
echo "x=$x"
f "${y:=assigned}"
echo "y=$y"
n=1
f $((n += 5))
echo "n=$n"
f "$(echo cmdsub)"
g() { local inner=$1; f "$inner"; }
g "pass through"
