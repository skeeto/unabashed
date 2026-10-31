shopt -s expand_aliases
alias a='echo ' b='if'
f() { a if then; a b fi; a { }; }
declare -f
