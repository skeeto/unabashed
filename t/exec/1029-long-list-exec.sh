# A generated script's long list runs as a short one does: lists nest
# down their left operands, and each element keeps its errexit context
# and its place for traps, as bash runs them. (The vos's 4 MiB store
# keeps a function body to a few thousand commands.)
n=12000
s=$(printf 'i=$((i+1));%.0s' $(seq $((n/4))))
i=0; eval "$s"; echo "count $i"
s=$(printf ':;%.0s' $(seq $n))
eval "$s echo semi"
{ eval "$s echo stage"; } | cat
{ eval "$s echo job"; } & wait
s=$(printf ': && %.0s' $(seq $n))
eval "$s echo and"
s=$(printf 'false || %.0s' $(seq $n))
eval "$s echo or"
# && and || in one chain, left to right
s=$(printf 'false && x || : && %.0s' $(seq $((n/4))))
eval "$s echo mix"
# break leaves a loop from the end of a long list
s=$(printf ':;%.0s' $(seq $((n/2))))
eval "while :; do $s break; $s echo no; done"; echo "break $?"
s=$(printf ':\n%.0s' $(seq $((n/3))))
eval "f() {
$s
echo f
}"
f
f | cat
f & wait
( f )
unset -f f
# set -e spares a left operand of && but not the last command
s=$(printf 'false && :;%.0s' $(seq $((n/2))))
( set -e; eval "$s :"; echo "errexit spared"; eval "$s false"; echo no ); echo "status $?"
# ERR runs once, for the failure at the list's end, at its line
trap 'echo "ERR at $LINENO: $BASH_COMMAND"' ERR
eval "$s
false"
trap - ERR
