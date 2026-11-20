# Details of the declaration builtins: an integer assignment's error
# names the builtin, -p lists only without names for export and
# readonly, and unset -n refuses a readonly variable
declare -i X
(declare X=1/0)
(f() { local -i Y=1/0; }; f)
(export X=1/0)
(read X <<< 1/0)
(printf -v X 1/0)
(eval 'X=1/0')
(eval 'declare X=1/0')
(X=1/0)
(declare -ai A; declare A=(1/0))
(declare -ai A; declare 'A[0]=1/0')
(declare -ai A; unset 'A[1/0]')
export -p nope; echo "export -p $?"
export -p Y=2; echo "export -p assigns $? [$Y]"; printenv Y
Z=1; readonly -p Z; declare -p Z
f() { :; }; export -pf f; declare -pF f
declare -pF nope; echo "-pF $?"
readonly R; unset -n R; echo "unset -n $?"
x=1; unset -n x; echo "unset -n plain [${x-unset}]"
