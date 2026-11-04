# A command substitution killed by a signal is not reported, and does
# not make the pipeline stage expanding it look killed. Nor does a
# subshell's last program when bash would not exec it, as with a trap
# set: the subshell reports it, and its status may be the trap's.
true | echo "[$(kill -PIPE $BASHPID)]"
echo "status $? ${PIPESTATUS[*]}"
true | echo "[$(kill -TERM $BASHPID)]" >&2
echo "status $? ${PIPESTATUS[*]}"
true | : "$(kill -TERM $BASHPID)" < /nonexistent
echo "status $? ${PIPESTATUS[*]}"
true | x=$(kill -TERM $BASHPID)
echo "status $? ${PIPESTATUS[*]}"
true | x=$(kill -KILL $BASHPID) | cat
echo "status $? ${PIPESTATUS[*]}"
true | (echo "[$(kill -TERM $BASHPID)]")
echo "status $? ${PIPESTATUS[*]}"
true | { echo "[$(kill -TERM $BASHPID)]"; }
echo "status $? ${PIPESTATUS[*]}"
true | echo "[$(kill -TERM $BASHPID)]" "$( (kill -INT $BASHPID) )"
echo "status $? ${PIPESTATUS[*]}"
( echo "[$(kill -TERM $BASHPID)]" )
echo "status $?"
x=$( (kill -TERM $BASHPID) ); echo "status $?"
echo "[$(kill -TERM $BASHPID)]"; echo "status $?"
true | (kill -TERM $BASHPID)
echo "status $? ${PIPESTATUS[*]}"
true | (trap "exit 3" EXIT; bash -c 'kill $$')
echo "status $? ${PIPESTATUS[*]}"
true | (trap "echo trap" EXIT; bash -c 'kill $$')
echo "status $? ${PIPESTATUS[*]}"
true | (trap "exit 3" EXIT; true; bash -c 'kill $$') | cat
echo "status $? ${PIPESTATUS[*]}"
(trap "exit 3" EXIT; bash -c 'kill $$'); echo "status $?"
(trap '' EXIT; bash -c 'kill $$'); echo "status $?"
(trap - EXIT; bash -c 'kill $$'); echo "status $?"
(trap 'echo usr1' USR1; bash -c 'kill $$'); echo "status $?"
(trap '' USR1; bash -c 'kill $$'); echo "status $?"
(trap 'echo err' ERR; bash -c 'kill $$'); echo "status $?"
(trap 'echo debug' DEBUG; bash -c 'kill $$'); echo "status $?"
f() { (trap 'echo ret' RETURN; bash -c 'kill $$'); }; f; echo "status $?"
g() { (trap 'echo ret' RETURN; true); bash -c 'kill $$'; }; g; echo "status $?"
