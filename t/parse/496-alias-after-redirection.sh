shopt -s expand_aliases
alias x='echo X'
f() { >out x a; 2>&1 x; <in >out x; >out y x; }
declare -f
