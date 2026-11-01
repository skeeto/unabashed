# ( ) subshell isolates variables and attributes
a=1
(a=2; echo "in: $a")
echo "out: $a"
(unset a; echo "in: ${a-unset}")
echo "out: $a"
(b=new)
echo "b: ${b-unset}"
(export a; declare -p a)
declare -p a
(readonly a)
a=3; echo "a now $a"
(declare -i n; n=1+2; echo "n=$n")
n=1+2; echo "n=$n"
( (a=deep; echo $a); echo $a )
