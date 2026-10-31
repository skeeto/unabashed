shopt -s expand_aliases
alias cs='echo $(echo'
f() { cs hi); echo after; }
declare -f
