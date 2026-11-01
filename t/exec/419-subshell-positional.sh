# Positional parameters and shift in subshells
set -- a b c
(shift; echo "$# $*")
echo "$# $*"
(set -- x; echo "$1")
echo "$1"
x=$(shift 2; echo $1)
echo "$x $1"
f() { (shift; echo "f: $@"); echo "f after: $@"; }
f p q r
