# DEBUG before a simple command pipeline stage runs in the parent,
# with its descriptors, before the stage starts: not in the stage
set -T
trap 'read -r l && echo "got $l" >&2' DEBUG
printf '1\n2\n3\n' | cat | cat; echo "1 $?"
trap - DEBUG
set +T
trap 'n=$((n+1)); echo "t $BASH_SUBSHELL $BASH_COMMAND" >&2' DEBUG
true | true | true; echo "2 $n"
f() { :; }
f | cat; echo "3 $n"
echo a | (cat) | { cat; }; echo "4 $n"
x=$(echo b | cat); echo "5 $x $n"
trap - DEBUG
set -T
trap 'echo "s $BASH_SUBSHELL $BASH_COMMAND" >&2' DEBUG
echo c | (cat); echo "6"
shopt -s lastpipe
echo d | cat; echo "7"
shopt -u lastpipe
trap - DEBUG
set +T
# Under extdebug, a stage the action skips does not start, and is left
# out of PIPESTATUS
shopt -s extdebug
trap 'case $BASH_COMMAND in skip*) false;; esac' DEBUG
skip() { echo "ran $1"; }
(exit 3) | skip 1; echo "8 $? ${PIPESTATUS[*]}"
skip 2 | (exit 4); echo "9 $? ${PIPESTATUS[*]}"
skip 3 | skip 4; echo "10 $? ${PIPESTATUS[*]}"
(exit 5) | skip 5 | (exit 6) | skip 6; echo "11 $? ${PIPESTATUS[*]}"
set -o pipefail
(exit 7) | skip 7; echo "12 $? ${PIPESTATUS[*]}"
set +o pipefail
g() {
    trap 'case $BASH_COMMAND in skip*) return 2;; esac' DEBUG
    (exit 8) | skip 8 | cat
    echo "in g $?"
}
g; echo "13 $? ${PIPESTATUS[*]}"
