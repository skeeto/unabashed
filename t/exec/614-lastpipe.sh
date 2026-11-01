# lastpipe: without job control the last stage runs in the shell itself
shopt -s lastpipe
echo a b | read x y; echo "1 $x $y"
echo a | { read v; w=$v$v; }; echo "2 $w"
echo sub | echo "3 $BASH_SUBSHELL"
true | x=assigned; echo "4 $x"
printf '%s\n' 1 2 3 | while read n; do last=$n; done; echo "5 $last"
f() { local l; echo loc | read l; echo "6 $l"; }; f; echo "6b ${l-unset}"
echo q | ( read z ); echo "7 ${z-unset}"
echo r | { read r1; } | cat; echo "8 ${r1-unset}"
true | false; echo "9 $? ${PIPESTATUS[*]}"
! true | true; echo "10 $? ${PIPESTATUS[*]}"
echo out | cd /; echo "11 $PWD"
echo n | read m < /dev/null; echo "12 [$m]"
x=$(echo c | read cc; echo "c=$cc"); echo "13 $x"
echo err | return 2>/dev/null; echo "14 $?"
g() { echo ret | return 5; echo "not here"; }; g; echo "15 $?"
for i in 1 2; do echo $i | break; done; echo "16 $i"
shopt -u lastpipe
echo a b | read x2 y2; echo "17 ${x2-unset}"
shopt -s lastpipe
h() { echo h | exit 6; echo "not here"; }; (h); echo "18 $?"
echo e | exit 7
echo "never"
