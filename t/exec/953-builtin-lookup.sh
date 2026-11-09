# The command name is looked up once its words are expanded: a first
# word that expands to nothing, a builtin disabled or enabled by a
# ${ ...; } in a later word, functions and declaration builtins
e=
$e echo "empty first word"
$e $e printf '%s\n' "two empty words"
: ${ enable -n :; }
echo "disabled meanwhile: $?"
enable :
enable -n :
: ${ enable :; }
echo "enabled meanwhile: $?"
f() { echo "f: $*"; }
f ${ enable -n echo; } arg
enable echo
echo() { builtin echo "fn: $*"; }
echo shadowed
unset -f echo
echo plain
x=1 $e declare -p x 2>&1
$e declare y=2; echo "y=$y"
decl=declare
$decl z=3 w; echo "z=$z"
command echo via command
builtin echo via builtin
%1 2>&1
echo "status $?"
