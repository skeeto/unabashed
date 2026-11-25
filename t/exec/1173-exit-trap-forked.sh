# bash's forked child for a pipeline stage's or job's simple command
# keeps the EXIT trap while it expands the words, so an expansion
# error there runs it in the child; the stage's redirections stay for
# an EXIT action it sets
trap 'echo exit=$?' EXIT
echo $((1.5)) & wait; echo "s=$?"
: | echo $((1.5)); echo "s=$?"
: | echo ${u?unset}; echo "s=$?"
: | x=$((1.5)) cat; echo "s=$?"
: | echo fine; echo "s=$?"
: | { echo $((1.5)); }; echo "s=$?"
f() { echo $((1.5)); }; : | f; echo "s=$?"
trap - EXIT
echo ---
: | trap 'echo bye' EXIT >f1; echo "[$(cat f1)]"
trap 'echo bye' EXIT >f2 | cat; echo "[$(cat f2)]"
trap 'echo bye' EXIT >f3 & wait; echo "[$(cat f3)]"
: | { trap 'echo bye' EXIT; } >f4; echo "[$(cat f4)]"
: | eval "trap 'echo bye' EXIT" >f5; echo "[$(cat f5)]"
( trap 'echo bye' EXIT >f6 ); echo "[$(cat f6)]"
