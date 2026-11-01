declare -i c=0 total=0
for w in a b c d; do c+=1; total+=c*10; done
echo "c=$c total=$total"
declare -i s
s=1; s=s*2; s=s*2; s=s*2; echo "s=$s"
declare -i z='5 > 3'; echo "z=$z"
declare -i q
read q <<< '3+4'; echo "read q=$q"
printf -v q '%s' '6*7'; echo "printf -v q=$q"
declare -i h=16#ff b=2#1111; echo "h=$h b=$b"
f() { local -i r; r=$1*2; echo "r=$r"; }
f 21
f '1+1'
declare -i u
u+=5; echo "u=$u"
declare -i k=3
k=k++ ; echo "k=$k"
