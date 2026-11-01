# ERR, DEBUG and RETURN: where they run, errtrace and functrace, $? and
# LINENO in them, BASH_COMMAND for DEBUG
trap 'echo "err $? at $LINENO"' ERR
false
f() { false; echo in f; }
f
if false; then :; fi
false || true
! false
false && true
true && false
echo "st $?"
( false )
x=$(false)
false | true
true | false
g() { return 3; }
g
set -o pipefail; false | true; set +o pipefail
for i in 1; do false; done
{ false; }
while false; do :; done
[[ -z x ]]
(( 0 ))
set -E
f
( false )
set +E
trap - ERR
echo ---
trap 'echo "dbg [$BASH_COMMAND] $?"' DEBUG
h() { echo inh; }
h
x=1
(( x ))
trap - DEBUG
echo ---
trap 'echo "ret ${FUNCNAME[0]}"' RETURN
k() { echo ink; return 4; }
k; echo "after $?"
set -T
k
trap - RETURN
echo ---
echo 'echo in source' > /tmp/s.sh
trap 'echo "ret $?"' RETURN
. /tmp/s.sh
f() { . /tmp/s.sh; }
f
trap - RETURN
trap 'echo "D $BASH_COMMAND"' DEBUG
for i in a b; do :; done
trap - DEBUG
trap 'echo e $?' ERR; trap 'echo exitt $?' EXIT
f2() { return 5; }; f2
false
