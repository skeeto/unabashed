shopt -s expand_aliases
alias x='echo X'
f() { echo x; 'x'; \x; "x"; a=1 x; }
declare -f
