shopt -s expand_aliases
alias two='echo one
echo two'
f() { two; }
declare -f
