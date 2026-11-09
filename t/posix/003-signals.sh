# Signals sent to the shell by another process (an inner bash's kill:
# the shell's own kill of $$ takes a shortcut of its own)
# QUIT: the shell itself ignores it, its subshells and programs do not
bash -c 'bash -c "kill -QUIT $$"; echo "QUIT ignored"'
bash -c 'trap "echo exit trap" EXIT; bash -c "kill -QUIT $$"; echo "QUIT ignored, EXIT trap set"'
bash -c 'trap "echo trapped QUIT" QUIT; bash -c "kill -QUIT $$"; trap - QUIT; bash -c "kill -QUIT $$"; echo "trap reset"'
bash -c '(kill -QUIT $BASHPID; echo no); echo "subshell $?"' 2>/dev/null
bash -c '(while [ ! -s pid ]; do sleep 1; done; kill -QUIT $(<pid)) & bash -c "echo \$\$ >pid; exec sleep 5"; echo "program $?"; rm pid' 2>/dev/null
bash -c '(sleep 1; bash -c "kill -QUIT $$") & sleep 2; echo "during a program $?"'
# With an EXIT trap set, the trap runs before the shell dies of a
# signal, the faults' included
for sig in ILL TRAP ABRT BUS FPE SEGV SYS; do
    bash -c 'trap "echo exit trap" EXIT; bash -c "kill -'$sig' $$"; echo "no"'
    echo "$sig $?"
done 2>/dev/null
