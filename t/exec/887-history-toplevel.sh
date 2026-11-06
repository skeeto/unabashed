# history with too many arguments jumps to the top level: a comsub
# ends there, and so do eval and the rest of the -c string
set -o history
x=$(history 1 2; echo sub)
echo "[$x] $?"
f() { history 1 2; echo in f; }
eval 'f; echo in eval'
echo not reached
