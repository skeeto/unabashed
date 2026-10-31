shopt -s expand_aliases
alias say='echo aliased'; f() { say hello; }
g() { say again; }
declare -f
