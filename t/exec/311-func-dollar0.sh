f() { echo "0=$0 1=$1"; }
f arg
function g { echo "g 0=$0"; }
g
h() { set -- a b; echo "h 0=$0 #=$#"; }
h
echo "top 0=$0"
