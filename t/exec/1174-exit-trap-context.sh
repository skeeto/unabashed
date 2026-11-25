# The exit builtin leaving -c, eval, source or a command substitution
# undoes what each ran before the EXIT trap, unless it was called in a
# function with that trap set
(trap 'echo exit' EXIT; eval 'exit 2' >f1); echo "s=$? [$(cat f1)]"
(trap 'echo exit' EXIT; eval 'exit 2 >f2'); echo "s=$? [$(cat f2)]"
echo 'exit 2' >s.sh
(trap 'echo exit' EXIT; . ./s.sh >f3); echo "s=$? [$(cat f3)]"
(trap 'echo exit' EXIT; f() { eval 'exit 2 >f4'; }; f); echo "s=$? [$(cat f4)]"
(trap 'echo exit $x' EXIT; f() { local x=1; exit 2 >f5; }; f); echo "s=$? [$(cat f5)]"
x=$(trap 'echo exit' EXIT; exit 2 >f6); echo "s=$? [$x] [$(cat f6)]"
(trap 'echo exit' EXIT; eval 'eval "exit 2 >f7"' >>f7); echo "s=$? [$(cat f7)]"
trap 'echo final' EXIT
exit 0 >f8
