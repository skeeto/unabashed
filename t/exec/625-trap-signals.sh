# Signal traps: kill to the shell itself runs the trap after kill, with
# $? as kill left it; traps inside traps; return in a trap; caught,
# ignored and default dispositions in subshells; deaths by signals
trap 'echo in; false; echo "trapst $?"' USR2
false
kill -USR2 $$
echo "st $?"
trap 'echo "trap sees $?"' USR2
(exit 7)
kill -USR2 $$ ; echo "st2 $?"
trap 'echo usr1; kill -USR2 $$; echo after2' USR1
kill -USR1 $$
echo next
trap 'echo "in usr1 $?"; return 9' USR1
g() { kill -USR1 $$; echo "g after $?"; }; g; echo "g $?"
trap - USR1 USR2
( kill $BASHPID; echo no ); echo "1 $?"
( trap 'echo exit2' EXIT; kill -INT $BASHPID; echo no ); echo "2 $?"
( trap 'echo exit3' EXIT; kill -TERM $BASHPID; echo no ); echo "3 $?"
{ ( trap 'echo exit4' EXIT; kill -KILL $BASHPID; echo no ); } 2>/dev/null; echo "4 $?"
{ ( kill -USR1 $BASHPID; echo no ); } 2>/dev/null; echo "5 $?"
( kill -CHLD $BASHPID; kill -WINCH $BASHPID; kill -CONT $BASHPID; kill -URG $BASHPID; echo alive ); echo "6 $?"
( trap '' TERM; kill $BASHPID; echo ignored ); echo "7 $?"
( trap 'echo caught' TERM; kill $BASHPID; echo after ); echo "8 $?"
trap 'echo parent caught' USR1
( kill -USR1 $BASHPID; echo "child survived?" ); echo "9 $?"
( kill -USR1 $$; echo "sent to parent" ); echo "10 $?"
trap - USR1
trap '' USR1
( kill -USR1 $BASHPID; echo "still ignored" ); echo "11 $?"
( trap - USR1; kill -USR1 $BASHPID; echo no ); echo "12 $?"
x=$(kill -TERM $BASHPID; echo no); echo "13 $? [$x]"
( kill -USR1 $BASHPID ) | cat; echo "14 $? ${PIPESTATUS[*]}"
true | ( kill -TERM $BASHPID ); echo "15 $? ${PIPESTATUS[*]}"
bash -c 'trap "echo exit \$?" EXIT; kill $$; echo after'; echo "16 $?"
bash -c 'kill -PIPE $$'; echo "17 $?"
bash -c 'kill -INT $$'; echo "18 $?"
trap 'echo int' INT
kill -INT $$
kill -s INT $$
kill -2 $$
trap 'echo term' TERM
kill $$
kill -0 $$; echo "19 $?"
# The EXIT trap runs before a death by a signal bash catches for it,
# but not by one it leaves at its default (STKFLT PROF IO PWR, real
# time), whoever sends it
for s in HUP ALRM VTALRM STKFLT PROF IO PWR RTMIN RTMAX; do
    { ( trap 'echo "exit $s"' EXIT; kill -s $s $BASHPID; echo no ); } 2>/dev/null
    echo "$s $?"
done
{ bash -c 'trap "echo exit" EXIT; kill -PROF $$; echo after'; } 2>/dev/null; echo "20 $?"
{ bash -c 'trap "echo exit" EXIT; bash -c "kill -PROF \$PPID"; echo after'; } 2>/dev/null; echo "21 $?"
