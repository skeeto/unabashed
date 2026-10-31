shopt -s expand_aliases
alias x='echo X'
f() { time -p x; time -p -- x; ! ! x; time ! x; coproc x; a |& x; }
declare -f
