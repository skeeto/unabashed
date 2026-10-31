shopt -s expand_aliases
alias two='echo one
echo two'
alias bad='echo a
fi'
f() { two; }
echo before
bad
