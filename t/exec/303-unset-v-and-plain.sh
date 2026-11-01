both() { echo "func both"; }
both=var
unset both
echo "after plain unset: [${both-unset}]"
both
unset both
both
echo "status $?"
onlyf() { echo onlyf; }
unset -v onlyf
onlyf
v=1
unset -v v
echo "[${v-unset}]"
unset -v nosuch
echo "status $?"
unset '1bad'
echo "status $?"
unset -v 'a b'
echo "status $?"
unset -f '1bad'
echo "status $?"
unset -fv x
echo "status $?"
