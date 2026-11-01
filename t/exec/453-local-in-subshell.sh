# local inside subshells and command substitution at top level
(local x=1 2>&1)
echo "st=$?"
y=$(local z 2>&1)
echo "y=[$y]"
f() { local a=f; (a=sub; echo $a); echo $a; b=$(local a=cs; echo $a); echo "$b $a"; }
f
