# A background job keeps the function bodies it was started with after
# the level that defined them ends: a job's own redefinition, and a
# function defined in a subshell
f() { f() { sleep 1; echo "inner $1"; }; f a & }
f & wait; sleep 2
(g() { sleep 1; echo "g ran"; }; g & ); g() { echo "outer g"; }; x=$(echo junk; echo more junk); sleep 2
g
