# ${ } funsubs: local, positional parameters, return
set -- p1 p2
x=${ echo "$1 $#"; }
echo "$x"
g() {
  local l=outer
  r=${ local l=inner; echo $l; }
  echo "$r $l"
}
g
y=${ set -- new; echo "$1"; }
echo "$y now $1"
f() { v=${ return 4; echo no; }; echo "after return $? v=[$v]"; }
f
echo "${ echo one; echo two; }"
echo "${ echo a
echo b
}"
