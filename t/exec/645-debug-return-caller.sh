# DEBUG and RETURN around functions: DEBUG as a traced function starts
# (at its body's line), RETURN at that line too and not for functions
# called from the DEBUG action, declare -ft, DEBUG before each
# expression of an arithmetic for, and the caller builtin
show() { echo "debug $1 ${FUNCNAME[1]} <$BASH_COMMAND>"; }
f() {
  echo body1
  echo body2
}
g()
{
  echo gbody
}
h() { caller; caller 0; caller 1; echo "status $?"; }
k() { h; }
caller; echo "top $?"
caller -z
set -T
trap 'show $LINENO' DEBUG
trap 'echo "return $LINENO ${FUNCNAME[0]}"' RETURN
f
g
for ((i=0; i<2; i++)); do :; done
trap - DEBUG RETURN
set +T
t() { echo traced; }
declare -ft t
trap 'echo "D $LINENO"' DEBUG
t
trap - DEBUG
k
h foo
