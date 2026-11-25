# A {var} that takes no assignment (GROUPS, FUNCNAME, BASH_SOURCE,
# BASH_LINENO, BASH_ARGV, BASH_ARGC) fails the redirection, as bash's
# check for one does, in any command. A dynamic variable a child's
# {var} assigns leaves the shell as it was, as bash's child does: the
# RANDOM seed, SECONDS, $0, BASH_SUBSHELL, COMP_WORDBREAKS and the
# alias and command tables. A subshell's commands see the assignment
# made, and its effects.
exec 2>&1
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$c"
    echo "status $?"
done <<'EOF'
true {GROUPS}>/dev/null; echo $?
sleep 0 {GROUPS}>/dev/null; echo $?
( : ) {GROUPS}>/dev/null; echo $?
exec {GROUPS}>/dev/null; echo $?
true {GROUPS[1]}>/dev/null; echo $?
declare -n r=GROUPS; true {r}>/dev/null; echo $?
f() { true {FUNCNAME}>/dev/null; echo $?; }; f
true {BASH_SOURCE}>/dev/null; true {BASH_LINENO}>/dev/null; true {BASH_ARGV}>/dev/null; sleep 0 {BASH_ARGC}>/dev/null
true {x}>/dev/null {GROUPS}>/dev/null {y}>/dev/null; echo $? ${x-unset} ${y-unset}
RANDOM=1; a=$RANDOM; RANDOM=1; sleep 0 {RANDOM}>/dev/null; b=$RANDOM; [ $a = $b ] && echo same
SECONDS=100; sleep 0 {SECONDS}>/dev/null; echo $SECONDS
SECONDS=100; ( echo $SECONDS ) {SECONDS}>/dev/null; echo $SECONDS
sleep 0 {BASH_ARGV0}>/dev/null; echo $0 $BASH_ARGV0
( echo $BASH_ARGV0 ) {BASH_ARGV0}>/dev/null; echo $BASH_ARGV0
sleep 0 {BASH_SUBSHELL}>/dev/null; echo $BASH_SUBSHELL
sleep 0 {COMP_WORDBREAKS}>/dev/null; echo "<$COMP_WORDBREAKS>"
( echo "<$COMP_WORDBREAKS>" ) {COMP_WORDBREAKS}>/dev/null
sleep 0 {BASH_ALIASES}>/dev/null {BASH_ALIASES[x]}>/dev/null; alias; echo "<$BASH_ALIASES>"
( echo $BASH_ALIASES ) {BASH_ALIASES}>/dev/null; alias
sleep 0 {BASH_CMDS[q]}>/dev/null; hash
true {BASH_ALIASES[x]}>/dev/null; alias
( alias ) {BASH_ALIASES[x]}>/dev/null; alias
( alias; echo "<$BASH_ALIASES>" ) {BASH_ALIASES}>/dev/null {BASH_ALIASES[y]}>/dev/null
( hash -t q ) {BASH_CMDS[q]}>/dev/null
( echo $0 $BASH_SUBSHELL; ( echo $0 $BASH_SUBSHELL ) ) {BASH_ARGV0}>/dev/null {BASH_SUBSHELL}>/dev/null; echo $0 $BASH_SUBSHELL
RANDOM=10; a=$RANDOM; ( echo $(( RANDOM == a )) ) {RANDOM}>/dev/null
EOF
