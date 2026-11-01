# ( ) subshells as loop bodies and conditions
i=0
while [ $i -lt 3 ]; do
    (i=99; echo "sub sees $i")
    i=$((i+1))
done
echo "i=$i"
for w in a b c; do (
    [ $w = b ] && exit 5
    echo "body $w"
); echo "status $?"; done
n=0
while (exit $n); do n=1; done
echo "n=$n"
for w in 1 2; do (echo "paren for $w"); done
x=0
( x=1; for i in 1 2; do x=$((x+i)); done; echo "inner x=$x" )
echo "outer x=$x"
( ( ( echo deep; exit 2 ); echo "mid $?"; exit 3 ); echo "outer $?" )
(cd /tmp; echo "cd in subshell")
echo "pwd unchanged: $PWD"
