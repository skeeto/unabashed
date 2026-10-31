shopt -s expand_aliases
alias dp='((echo a) )'
f() { ((echo b) ); }
g() { ((c);(d)); }
h() { dp; }
declare -f
