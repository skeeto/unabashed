shopt -s expand_aliases
alias x='echo X'
f() { x | x && x || x; x & x; ! x; time x; (x); { x; }; if x; then x; fi; }
declare -f
