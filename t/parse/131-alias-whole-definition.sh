shopt -s expand_aliases
alias mkf='g() { echo from alias; }'
mkf
declare -f
