# Traps set inside functions, return in trap actions, BASH_TRAPSIG, and
# -p with -P
echo "[${BASH_TRAPSIG-unset}]"
trap 'echo "u $BASH_TRAPSIG"' USR1; kill -USR1 $$
trap 'echo "e $BASH_TRAPSIG"' ERR; false
trap - ERR
trap 'echo "d $BASH_TRAPSIG"' DEBUG; :
trap - DEBUG
f() { :; }; trap 'echo "r $BASH_TRAPSIG"' RETURN; set -T; f; set +T; trap - RETURN
trap -p -P; echo $?
declare -p BASH_TRAPSIG
BASH_TRAPSIG=5; echo "$BASH_TRAPSIG"
trap 'echo "x $BASH_TRAPSIG"' EXIT
f() { trap 'echo "fd [$BASH_COMMAND]"' DEBUG; echo in f; }
f
echo after f
g() { echo in g; }
g
trap - DEBUG
r() { trap 'echo "returning from ${FUNCNAME[0]}"' RETURN; echo in r; }
r
r2() { echo in r2; }
r2
trap - RETURN
e() { trap 'echo "err in e $?"' ERR; false; echo after false; }
e
false
trap - ERR
setexit() { return "$1"; }
trap 'setexit 222; echo $? ; return' USR1
process() { kill -USR1 $$; }
process
echo exit=$?
invoke() { kill -USR1 $$; return 222; }
trap 'setexit 111; return' USR1
invoke; echo "invoke $?"
handler() { setexit 111; return; }
trap 'handler; stat=$?; return' USR1
invoke; echo "stat $stat"
trap 'echo BASH_TRAPSIG = $(kill -l $BASH_TRAPSIG); return 7' USR1
func() { kill -USR1 $$; }
func; echo func=$?
trap - USR1
ReturnFalse() { false ; return; }
trap 'ReturnFalse && echo "woops"' EXIT
(exit 0)
