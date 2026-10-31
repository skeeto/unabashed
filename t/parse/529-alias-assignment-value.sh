shopt -s expand_aliases
alias as='x=1 ' as2='x=2' y='echo Y'
f() { as y; as; as2 y; }
declare -f
