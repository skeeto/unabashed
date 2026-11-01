# Many command substitutions and subshells in a loop keep state right
i=0
acc=
while [ $i -lt 500 ]; do
  v=$(echo $i)
  (acc=bad; i=999)
  acc=$v
  i=$((i+1))
done
echo "$i $acc"
f() { echo "r$1"; }
s=
for k in 1 2 3; do s="$s$(f $k)${ f $k; }"; done
echo "$s"
echo $( ( ( ( echo four-deep ) ) ) )
