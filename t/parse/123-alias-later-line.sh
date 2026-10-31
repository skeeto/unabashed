shopt -s expand_aliases
alias say='echo aliased'
f() { say hello; }
declare -f
