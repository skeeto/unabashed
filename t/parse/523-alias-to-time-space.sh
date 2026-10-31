shopt -s expand_aliases
alias t='time ' x='echo X' p='-p'
f() { t x; t -p x; t p x; }
declare -f
