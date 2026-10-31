shopt -s expand_aliases
alias bs='echo a\'
alias bb='echo c\\'
f() { bs b; bb d; }
declare -f
