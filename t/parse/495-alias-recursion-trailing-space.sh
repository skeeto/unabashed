shopt -s expand_aliases
alias echo='echo ' a='echo' b='B'
f() { echo a b; echo echo b; a b; }
declare -f
