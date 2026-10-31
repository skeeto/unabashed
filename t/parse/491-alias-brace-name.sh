shopt -s expand_aliases
alias {='echo brace'
f() { { x; }
g() { echo {; }
declare -f
