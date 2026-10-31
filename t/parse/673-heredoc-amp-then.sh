# nofixedpoint: bash's own output does not reparse
g() { if cat <<E &
x
E
then :; fi; }
declare -f
