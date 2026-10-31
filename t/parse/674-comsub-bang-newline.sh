# nofixedpoint: bash's own output does not reparse
shopt -s extglob
g()case "$(!
)" in
esac
declare -f
