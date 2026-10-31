shopt -s expand_aliases
alias p='echo a |' r='echo b >'
f() { p| echo c; r> out; p cat; }
declare -f
