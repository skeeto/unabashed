shopt -s expand_aliases
alias n='! ' nn='!' x='echo X'
f() { n x; n n x; nn x; nn nn x; }
declare -f
