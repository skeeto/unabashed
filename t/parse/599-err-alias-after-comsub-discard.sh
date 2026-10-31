shopt -s expand_aliases
alias e='echo'
f() { x=$(a=(;)); }
f() { e hi; }
declare -f
