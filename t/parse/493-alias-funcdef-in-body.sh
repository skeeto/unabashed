shopt -s expand_aliases
alias mk='g() { echo g; }'
f() { mk; echo after; }
declare -f
