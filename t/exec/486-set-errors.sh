set -o nosuch
echo "st=$?"
set +o nosuch
echo "st=$?"
set -Z
echo "st=$?"
set -o errexit nosuch
echo "st=$?"
set -- a b
echo "args=$# $1 $2"
set -
echo "after dash=$# $-"
set -- -x
echo "args=$1"
set +
echo "plus st=$?"
set -e -- p q
echo "pq=$1$2 $-"
set +e
set x y z
echo "plain=$*"
set --
echo "empty=$#"
