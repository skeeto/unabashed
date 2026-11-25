# A {var} is assigned once its descriptor is open, so the redirections
# after it see it, in a child's redirections too (a null command's, a
# program's, a subshell's), though the shell then keeps nothing.
cd "$(mktemp -d)"
{x}>f1 &>"$x"; echo "status $? x=${x-unset}"
{x}>f2 2>"$x.a"; echo "status $? x=${x-unset}"
cat </dev/null {y}>f3 >"$y.b"; echo "status $? y=${y-unset}"
( echo sub ) {z}>f4 >"$z.c"; echo "status $? z=${z-unset}"
: {w}>f5 >"$w.d"; echo "status $? w=$w"
cat </dev/null {v}>f6 >"$(echo "$v.e")"; echo "status $?"
cat </dev/null {u}>f7 {t}>"$u.f"; echo "status $? u=${u-unset} t=${t-unset}"
ls
cat 10.d 2>&1
