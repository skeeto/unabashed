shopt -s expand_aliases
alias x='y'
f() { echo > x; x > x; x <<< x; x 2>&1 x; }
declare -f
