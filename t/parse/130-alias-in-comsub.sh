shopt -s expand_aliases
alias q='echo quiet'
f() { x=$(q one); y=`q two`; z="$(q three)"; }
declare -f
