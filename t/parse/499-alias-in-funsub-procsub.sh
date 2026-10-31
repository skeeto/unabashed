shopt -s expand_aliases
alias q='echo Q'
f() { a=${ q 1; }; b=${| q 2; }; cat <(q 3) >(q 4); c=$( (q 5) ); }
declare -f
