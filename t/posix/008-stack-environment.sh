# A large environment fills the top of the main thread's stack before
# the shell runs: recursion that exhausts what is left kills only the
# subshell, as in bash. The shell once counted the whole stack limit as
# its own and died itself, by a real SIGSEGV.
big=$(printf '%050000d' 0)
B1=$big B2=$big B3=$big B4=$big bash -c '(f() { f; }; f); echo "status $?"' 2>/dev/null
echo "outer $?"
