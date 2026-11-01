X=1 :
echo "bash mode X=[${X-unset}]"
set -o posix
Y=2 :
echo "posix mode Y=[${Y-unset}]"
f() { echo "f Z=$Z"; }
Z=3 f
echo "posix after func Z=[${Z-unset}]"
W=4 eval 'echo "eval W=$W"'
echo "W=[${W-unset}]"
