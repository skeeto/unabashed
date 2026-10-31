shopt -s expand_aliases
alias a='b' b='echo ' x='X'
alias e='' y='echo Y'
f() { a x; e y; e if true; then e :; fi; }
declare -f
