# exit inside $( ) only leaves the substitution
x=$(echo before; exit 3; echo after)
echo "x=[$x] status=$?"
y=$(exit)
echo "status=$?"
false
y=$(exit)
echo "after false status=$?"
z=$(f() { exit 5; }; f; echo not)
echo "z=[$z] $?"
w=$(return 2 2>&1)
echo "w=[$w]"
echo still running
