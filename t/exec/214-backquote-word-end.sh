# Backquoted command substitution at the start, middle and end of a
# word, alone, and inside brace expansion.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show `echo a`
show x`echo b`
show `echo c`y
show x`echo d`y
show "`echo e f`"
show `echo g h`
show `echo i``echo j`
show {1,`echo k`}
show `echo '*'`
x=`echo l`
show "$x"
