# source of missing files and bad arguments
. /tmp/nonexistent.sh
echo "status $?"
source /tmp/nope 2>&1
echo "status $?"
source 2>&1
echo "no arg: $?"
. 2>&1
echo "dot no arg: $?"
source /tmp 2>&1
echo "dir: $?"
echo still alive
