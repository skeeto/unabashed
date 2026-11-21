# A builtin's failed write is reported by the builtin that wrote, also
# when started by `builtin` or `command`, which were named instead, and
# in every kind of command around it. (A write that fails only as the
# shell's gathered output goes out, at the file's close, cannot be
# made on demand: a unit test, test_latewrite, covers that.)
builtin echo x >/dev/full; echo "builtin $?"
command printf y >/dev/full; echo "command $?"
command builtin echo z >/dev/full; echo "both $?"
{ echo a; echo b; } >/dev/full; echo "group $?"
f() { echo in f; }; f >/dev/full; echo "function $?"
for i in 1 2; do echo $i; done >/dev/full; echo "for $?"
(echo sub) >/dev/full; echo "subshell $?"
(exec >/dev/full; echo e1; echo e2; true); echo "exec $?"
bash -c 'exec >/dev/full; echo last'; echo "exit $?"
pwd >/dev/full; echo "pwd $?"
help help >/dev/full; echo "help $?"
