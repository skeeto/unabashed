# personality: native
# Hand-checked baseline against bash 5.3: recursion that exhausts the
# stack kills the subshell by SIGSEGV, without its EXIT trap, and the
# parent goes on with status 139.
(trap 'echo exit trap' EXIT; f() { f; }; f) 2>/dev/null
echo "status $?"
x=$( (f() { f; }; f; echo unreached) 2>&1 )
echo "status $? ${x##* }"
