x=$(nosuch1)
y=$(echo a
nosuch2
echo b)
echo "$y"
f() {
  nosuch3
}
f
eval 'nosuch4
nosuch5'
z=`nosuch6`
echo $(
  echo deep; nosuch7
)
echo "${bad x}" &&
  echo no
if nosuch8; then :; fi
x=$(echo "in:$LINENO"
echo "in2:$LINENO"); echo "$x"
echo $(echo "a:$LINENO"
echo "b:$LINENO")
echo $(
echo "c:$LINENO")
echo one \
  $(echo "d:$LINENO")
echo "e:$LINENO" $(echo "f:$LINENO")
y=1 \
z=$(echo "g:$LINENO")
echo $z
eval 'echo "h:$LINENO"
echo "i:$LINENO"'
f() {
  echo "j:$LINENO" $(echo "k:$LINENO")
}
f
echo end
