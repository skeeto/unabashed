declare -l lo=HeLLo
declare -u up=HeLLo
echo "$lo $up"
lo="MiXeD Case 123"; up="mixed case 123"
echo "$lo|$up"
declare -u x
x=abc; echo "$x"
declare -l x
x=ABC; echo "after -l: $x"
declare -p x
declare -ul both=AbC
echo "both: $both"
declare -p both
declare +u up
up=abc; echo "plus u: $up"
lo+=XYZ; echo "append: $lo"
f() { local -u lu=quiet; echo "$lu"; }
f
v=already
declare -u v
echo "existing not changed: $v"
v=$v; echo "reassigned: $v"
