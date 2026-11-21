# A function that does not inherit DEBUG, ERR or RETURN starts with it
# unset, and its caller's comes back on return unless it set its own:
# an action it sets runs in it however short (the same size as the
# caller's, or the same text), and stays after it returns
trap 'echo err1' ERR
f() { trap 'echo err2' ERR; false; echo "f done"; }
f
false
f() { trap 'echo err2' ERR; false; }
f
trap 'echo err1' ERR
f() { trap 'echo err1' ERR; false; echo "same done"; }
f
echo "-- unset in a function, and put back"
f() { trap - ERR; false; echo "unset done"; }
f
false
echo "-- a callee's sets for its caller; its unset is undone"
trap 'echo err1' ERR
g() { trap 'echo errg' ERR; }
f() { false; g; false; echo "f done"; }
f
trap 'echo err1' ERR
g() { trap - ERR; false; }
f() { trap 'echo errf' ERR; g; false; }
f
trap -p ERR
trap - ERR
echo "-- listed only where it runs"
trap 'echo err1' ERR
trap 'echo ret1' RETURN
f() { trap -p; echo listed; }
f
trap - ERR RETURN
echo "-- DEBUG"
trap 'echo "outer-dbg"' DEBUG; f() { trap 'echo "inner-dbg"' DEBUG; echo in-f; echo in-f2; }; f
trap - DEBUG
trap 'echo "dbg $BASH_COMMAND"' DEBUG
g() { echo in-g; }
declare -ft g
f() { g; set -T; g; echo in-f; }
f
set +T
trap - DEBUG
echo "-- RETURN"
trap 'echo ret1' RETURN
f() { trap 'echo ret2' RETURN; echo in-f; }
f
trap 'echo "ret0 $FUNCNAME"' RETURN
g() { trap 'echo "ret-g $FUNCNAME"' RETURN; echo in-g; }
f() { g; echo in-f; }
f
trap - RETURN
f() { trap 'echo newret' RETURN; }
trap 'f; trap - DEBUG' DEBUG
echo after-debug
trap -p RETURN
trap - RETURN
echo "-- recursion"
trap 'echo err0' ERR
f() { if [ $1 = 0 ]; then false; return; fi; trap "echo err$1" ERR; f $(($1-1)); false; }
f 2
trap - ERR
echo "-- exit in a function"
trap 'echo err1' ERR
trap 'echo EXIT; trap -p ERR' EXIT
f() { exit 3; }
f
