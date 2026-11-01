# Reports of foreground jobs killed by signals: none for SIGINT and
# SIGPIPE, nor in substitutions; a subshell's last program is the
# subshell's death, as bash execs it; a pipeline reports with its last
# stage
x=$(bash -c 'kill $$'); echo "1 $?"
x=$(true; bash -c 'kill $$'); echo "2 $?"
x=$(bash -c 'kill $$'; true); echo "3 $?"
( bash -c 'kill $$' ); echo "4 $?"
( true; bash -c 'kill $$' ); echo "5 $?"
( bash -c 'kill $$'; true ); echo "6 $?"
cat < <(bash -c 'kill $$'; echo hi)
cat < <(bash -c 'kill $$')
{ bash -c 'kill $$'; } | cat; echo "7 $?"
bash -c 'kill $$' | cat; echo "8 $?"
f() { bash -c 'kill $$'; }; f; echo "9 $?"
true | bash -c 'kill $$'; echo "10 $? ${PIPESTATUS[*]}"
bash -c 'kill $$' | bash -c 'kill $$'; echo "11 $? ${PIPESTATUS[*]}"
true | ( true; bash -c 'kill $$' ); echo "12 $?"
true | { bash -c 'kill $$'; }; echo "13 $?"
( bash -c 'kill -INT $$' ); echo "14 $?"
nosuch $(bash -c 'kill $$') 2>/dev/null; echo "15 $?"
exec bash -c 'kill $$'
