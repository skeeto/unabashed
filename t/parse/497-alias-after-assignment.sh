shopt -s expand_aliases
alias x='echo X'
f() { a=1 x; a=1 b=2 x y; a=(1 2) x; a=1 >out x; a+=1 x; }
declare -f
