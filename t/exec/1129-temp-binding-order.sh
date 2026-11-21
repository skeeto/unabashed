# bash binds a simple command's temporary assignments after its words
# expand (a declaration's arrays assigned with them) and before its
# trace and its redirections; a variable's hook (the setlocale warning)
# runs as it is bound only before a builtin or a function, and again as
# it is unbound after any command. bash's child for a stage or job
# running a builtin leaves them bound until the next simple command, its
# EXIT action's first, ends; a function, eval and the like keep them.
echo a; LC_ALL=y : >/nonexistent/x
readonly ro=1
echo b; LC_ALL=y declare ro=(z); echo notreached
echo c; set -x; LC_ALL=y : 2>&1; set +x
LC_ALL=x
echo d; LC_ALL=y : >/nonexistent/x
echo e; LC_ALL=y env >/dev/null
echo f; LC_ALL=y command env >/dev/null
echo g; LC_ALL=y trap 'echo "1 $LC_ALL" >&2; echo "2 $LC_ALL" >&2' EXIT | cat
fn() { trap 'echo "f1 $LC_ALL" >&2; echo "f2 $LC_ALL" >&2' EXIT; }
echo h; LC_ALL=y fn | cat
echo i; LC_ALL=y eval 'trap "echo e1 \$LC_ALL >&2; echo e2 \$LC_ALL >&2" EXIT' | cat
echo j; LC_ALL=y fn >/nonexistent/x
LC_ALL=
echo k; K=1 L=$K M=$(echo "<$L>" >&2) : $(echo "w$K" >&2)
echo end
