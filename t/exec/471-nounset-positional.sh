set -u
echo "count=$#"
echo "all=[$@]"
echo "star=[$*]"
for a in "$@"; do echo "arg $a"; done
echo "zero=$0"
(echo "one=$1"; echo after1); echo "st=$?"
(echo "ten=${10}"; echo after10); echo "st=$?"
set -- a b
echo "one=$1 two=$2"
(echo "three=$3"; echo after3); echo "st=$?"
echo "${3-none}"
f() { echo "f1=$1"; echo "f2=$2"; }
(f only; echo afterf); echo "st=$?"
shift 2
echo "now=$#"
echo "atlen=${#@}"
(echo "$1"); echo "st=$?"
