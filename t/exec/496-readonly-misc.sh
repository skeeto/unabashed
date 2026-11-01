readonly a b=2
echo "a=${a-unset} b=$b"
a=1
echo "st=$?"
readonly -p | { while read -r l; do case $l in *' a'|*' b='*) echo "$l";; esac; done; }
f() { echo f; }
readonly -f f
f() { echo g; }
echo "redef st=$?"
f
unset -f f
echo "unsetf st=$?"
declare -r c=3
c=4
echo "c st=$? c=$c"
(( b = 5 ))
echo "arith st=$? b=$b"
let b=6
echo "let st=$? b=$b"
b=7 true
echo "prefix-builtin st=$?"
echo end
