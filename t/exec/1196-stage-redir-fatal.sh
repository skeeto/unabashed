# A fatal error in the redirections of a forked compound command (a
# pipeline stage, a job) comes before its child's own catch, as bash's
# execute_in_subshell applies them first: from -c the child ends with
# 127, as for a program's redirections; in a script, with 1, as do a
# ( list )'s and a program's own there. Inside the command, the
# child's catch gives 1.
cat >s.sh <<'EOF2'
set -u
( : ) 10>"$x"; echo "a $?"
cat 10>"$x"; echo "b $?"
true | ( : ) 10>"$x"; echo "c $? ${PIPESTATUS[*]}"
true | { :; } 10>"$x"; echo "d $? ${PIPESTATUS[*]}"
( : ) 10>"$x" | true; echo "e $? ${PIPESTATUS[*]}"
{ :; } 10>"${x?}" & wait $!; echo "f $?"
true | while :; do break; done 10>"$x"; echo "g $?"
f() { :; } 10>"$x"; true | f; echo "h $?"
true | ( echo $x ); echo "i $?"
true | { :; } 10>/nope/x; echo "j $?"
EOF2
bash s.sh 2>&1; echo "script $?"
bash -c "$(cat s.sh)" 2>&1; echo "-c $?"
rm -f s.sh
