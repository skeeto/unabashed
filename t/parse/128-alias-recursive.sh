shopt -s expand_aliases
alias ls='ls -F'
alias a='b'
alias b='a x'
f() { ls; a; }
declare -f
