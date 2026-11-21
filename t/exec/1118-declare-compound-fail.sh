# A declaration builtin's literal name=(...) is declared and assigned
# while the words expand, as bash's expand_declaration_argument: when
# that fails the command is abandoned, unless declare, typeset or local
# runs in a function (-g or not), where it is reported again and the
# command goes on. A function's name abandoned with it is not left to
# prefix a later message.
readonly ro=1
declare ro=(z); echo notreached
echo "next $?"
export ro=(z); echo notreached
readonly ro=(z); echo notreached
typeset ro+=(z); echo notreached
declare ro=(); echo notreached
declare -x ro=(z); echo notreached
{ declare ro=(z); echo notreached; }; echo notreached
for i in 1 2; do declare ro=(z); echo notreached; done
declare ro=(z) && echo notreached || echo notreached
( declare ro=(z); echo notreached ); echo "subshell $?"
echo "<$(declare ro=(z); echo notreached)>"
eval 'declare ro=(z); echo notreached'; echo "eval $?"
f() { declare ro=(z); echo "f $?"; }; f; echo "after f $?"
g() { declare -g ro=(z); echo notreached; }; g; echo notreached
echo "next $?"
declare -i ro=(z)
h() { declare -g -i ro=(z); echo "h $?"; }; h; echo "after h $?"
k() { export ro=(z); echo notreached; }; k; echo notreached
m() { export -a ro=(z); echo notreached; }; m; echo notreached
n() { local -r lr=1; declare lr=(z); echo notreached; }; n; echo notreached
p() { local -r lr=1; local lr=(z); echo notreached; }; p; echo notreached
declare -A as=([k]=v)
q() { export -a as=(1); echo notreached; }; q; echo notreached
r() { declare -g -a as=(1); echo "r $?"; }; r; echo "after r $?"
declare -i ro=(z)
echo end
