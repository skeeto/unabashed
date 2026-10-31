shopt -s expand_aliases
alias lc='echo a\
b c'
alias le='echo d\
'
f() { lc x; le y; }
declare -f
fi
