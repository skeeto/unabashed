# declare -F under extdebug: bash gives a function the line its parser
# marked last, at the name after "function" and at the ) of "name ( )",
# with one line kept, not a stack: a nested definition marked twice
# moves the enclosing one to its own line. Imports count from 0 alike.
shopt -s extdebug
f1() { :; }
f2()
{
  :
}
f3 ( )

{ :; }
function f4 { :; }
function f5 () { :; }
function f6

{ :; }
f7() {
  :
  function g7() { :; }
  :
}
f8() {
  function g8 { :; }
  h8() { :; }
}
f9() {
  h9() { :; }
}
function f10 {
  function g10 () {
    :
  }
}
f11() (
  :
)
f12() { :; }; f13() { :; }
eval 'f14() {
:; }'
f15() { function g15 { :; }; }; f16() { :; }
declare -F f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14 f15 f16
f7; f8; f9; f10; f15
declare -F g7 g8 h8 h9 g10 g15
k1() {
  x=$(function m1() { :; }
  )
}
k2() {
  : $(
  m2() { :; }
  )
}
k3() (
  function m3() { :; }
)
printf 'k5() {\n:\n}\nfunction k6\n{ :; }\n' > src; . ./src
k7() { :; } >/dev/null
function k8 ( ) { :; }
function k9 ( echo; )
k10() {
  case x in (a) :;; esac
}
k11() { function m11 () { :; }; function m12 { :; }; }
k13() {
  k14() {
    function m14() { :; }
  }
}
declare -F k1 k2 k3 k5 k6 k7 k8 k9 k10 k11 k13
k13; declare -F k14
x=$(k15() { :; }
declare -F k15)
echo "$x"
export -f f2 f4 f7 f8 k1 k11 k13
bash -c 'shopt -s extdebug; declare -F f2 f4 f7 f8 k1 k11 k13; f7; declare -F g7'
