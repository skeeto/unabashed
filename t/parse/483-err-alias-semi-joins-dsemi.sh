shopt -s expand_aliases
alias s='echo a;'
echo before
f() { s; }
