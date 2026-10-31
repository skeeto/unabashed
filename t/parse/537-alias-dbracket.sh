shopt -s expand_aliases
alias c='[[ -n' c2='[[ ' n='-n' x='y'
f() { c x ]]; c2 n x ]]; [[ x == x ]]; }
declare -f
