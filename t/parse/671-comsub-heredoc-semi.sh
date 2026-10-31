# nofixedpoint: bash's own output does not reparse
g() { echo $(a <<F ;
b
F
c); }
declare -f
