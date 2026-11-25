# A script without #! runs in a child bash forks for it: its $$ and
# BASHPID are the child's, its PPID the pid of the shell that ran it,
# and it has none of that shell's jobs. Where bash would exec the
# script in place (the last command of -c, a comsub, a job), $$ is
# that process's and PPID its parent's.
cat > s <<'END'
echo "self $(($$==BASHPID)) new $(($$!=P)) parent $(($PPID==PB)) top $(($PPID==P))"
kill -0 $PPID && echo alive
kill -0 $$ && echo self
jobs; echo "jobs $?"; wait; echo "wait $?"
wait %1 2>/dev/null; echo "wait1 $?"
(echo "sub $(($BASHPID!=$$)) $(($PPID==PB))")
echo "bang $((${!:-0}==B))"
END
chmod +x s
P=$$ PB=$BASHPID; export P PB
sleep 1 &
B=$!; export B
./s
echo "after $?"
(PB=$BASHPID; export PB; ./s; :)
x=$(PB=$BASHPID; export PB; ./s); echo "$x"
PB=$BASHPID ./s & wait $!
./s | cat
echo "-- exec"
(PB=$BASHPID; export PB; exec ./s)
bash -c 'PB=$$; export PB; ./s; :'
kill %1 2>/dev/null; wait
