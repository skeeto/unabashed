# A {var} redirection bash applies in a child (a program's, a
# subshell's, a coproc's, a null command's) assigns the variable and
# opens the descriptor there only: the shell keeps neither, while a
# builtin's, a function's, a group's and exec's stay. A subshell's
# commands see the variable, with its attributes.
cd "$(mktemp -d)"
sleep 0 {x}>f; declare -p x
cat </dev/null {x}>f; declare -p x
x=5 cat </dev/null {x}>f; declare -p x
command cat </dev/null {x}>f; declare -p x
nosuch {x}>f; declare -p x
{x}>f; declare -p x
x=7; sleep 0 {x}>f; declare -p x
export e=1; printenv e {e}>f; declare -p e
declare -i i=3; cat </dev/null {i}>f; declare -p i
a=(1 2); cat </dev/null {a[1]}>f {a}>g; declare -p a
declare -n r=t; cat </dev/null {r}>f; declare -p t
readonly ro=1; cat </dev/null {ro}>f; echo "status $?"
unset x
( declare -p x ) {x}>f; declare -p x
x=4; ( echo "in $x"; x=8 ) {x}>f; echo "out $x"
( printenv e ) {e}>f; printenv e
( declare -p i; i+=1; echo $i ) {i}>f; declare -p i
( declare -p a ) {a[1]}>f; declare -p a
unset b; ( declare -p b ) {b[3]}>f; declare -p b
( exit 3 ) {x}>f; echo "status $?"
coproc C { :; } {y}>f; wait; declare -p y
# The child's descriptor is not left open: the next {var} gets 10
unset x; sleep 0 {x}>f; : {z}>g; echo "z=$z"
: {w}>h; echo "w=$w"
f() { :; }; f {v}>k; echo "v=$v"
{ :; } {u}>l; echo "u=$u"
exec {t2}>m; echo "t2=$t2"
exec 6>f; cat </dev/null {k2}>&6-; echo "k2=${k2-unset}"; echo six >&6; cat f
