# A command substitution killed by a signal is not reported, and does
# not make the pipeline stage expanding it look killed.
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
