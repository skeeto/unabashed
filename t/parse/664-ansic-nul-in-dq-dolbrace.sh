# nofixedpoint: bash's own output does not reparse
f() { echo "${y:=$'\0'}x" z; }
g() { echo $'a\0b'c "${y-$'p\0q'r}s"t; }
declare -f
