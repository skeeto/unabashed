# A builtin that is the whole command of bash's forked child (a
# pipeline stage or a & job) exits with bash's internal status: 4
# (EX_BADASSIGN) for a declaration's assignment errors, 7 (EX_UTILERROR)
# for unset's special builtin errors. Elsewhere they are 1.
readonly R
declare -ir I=1
: | readonly R=; echo "readonly $?"
: | export R=1; echo "export $?"
: | declare R+=x; echo "declare $?"
: | typeset 1bad; echo "typeset $?"
: | declare -n N=1bad; echo "nameref $?"
: | declare -n N=N; echo "self $?"
: | declare -n N=; echo "empty $?"
: | export 'A[1]=2'; echo "export sub $?"
: | readonly 1bad=1; echo "readonly word $?"
: | readonly R=1 1bad; echo "mixed $?"
: | declare -a R; echo "attribute $?"
: | declare -n R=y; echo "attribute nameref $?"
: | command readonly R=1; echo "command $?"
: | unset R; echo "unset $?"
: | builtin unset -v 1bad; echo "unset name $?"
: | unset BASH_ARGV; echo "unset stack $?"
: | unset -n R; echo "unset -n $?"
true | unset R | true; echo "stage ${PIPESTATUS[@]}"
readonly R=1 & wait $!; echo "async $?"
f() { : | readonly R=1; echo "function $?"; }; f
f() { : | local R=1; echo "shadow $?"; }; f
f() { local -r L; : | local L=1; echo "local $?"; }; f
: | { readonly R=1; }; echo "group $?"
: | eval readonly R=1; echo "eval $?"
v=$(readonly R=1); echo "comsub $?"
(readonly R=1); echo "subshell $?"
coproc readonly R=1; wait $!; echo "coproc $?"
readonly R=1; echo "plain $?"
unset R; echo "plain unset $?"
(set -o posix; : | unset R; echo "posix $?")
