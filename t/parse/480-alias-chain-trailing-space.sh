shopt -s expand_aliases
alias a='b ' b='echo ' c='hi ' d='there'
f() { a c d e; }
declare -f
