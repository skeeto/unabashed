shopt -s expand_aliases
alias ar='(( x' ar2='((x+1))'
f() { ar + 1 )); ar2; ar2 && ar2; }
declare -f
