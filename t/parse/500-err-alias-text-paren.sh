shopt -s expand_aliases
alias bad='echo a (b'
echo before
f() { bad; }
