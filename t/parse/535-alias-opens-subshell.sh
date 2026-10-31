shopt -s expand_aliases
alias sp='( echo in' gp='{ echo in'
f() { sp ); gp; }; }
declare -f
