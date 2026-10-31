# nofixedpoint: bash's own output does not reparse
f() {
<<'E'
abc
E
}
declare -f
