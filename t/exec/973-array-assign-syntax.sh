# A declaration's quoted (...) list is parsed as bash's "array assign":
# a syntax error is reported and abandons the command
declare -a A='(a "b c" [3]=d)'; declare -p A
declare -a A='( a # comment
b )'; declare -p A
declare -a A='([1)'; echo not reached
echo "st $?"
declare -a A='(a "b)'; echo not reached
declare -a A='(a ;b)'
declare -a A='(a |b)'
declare -a A='(a (b))'
declare -A B='([x)'
A='([1]=a [2)'; declare -a A="$A"
declare -a C; C+='([1)'; declare -p C
f() { local -a A='([1)'; echo in; }; f; echo not reached
f; echo "after f $?"
eval "declare -a A='([1)'"; echo "eval $?"
(declare -a A='([1)'; echo in); echo "subshell $?"
(set -o posix; declare -a A='([1)'; echo in); echo "posix $?"
