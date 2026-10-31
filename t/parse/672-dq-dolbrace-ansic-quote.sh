# nofixedpoint: bash's own output does not reparse
g() { echo "${x:-$'\''}"; }
declare -f
