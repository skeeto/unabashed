# source searches PATH for names without a slash; source -p
mk() { while IFS= read -r l; do printf '%s\n' "$l"; done; }
mkdir_free=1
mk > /tmp/onpath.sh <<'END'
echo "found onpath $1"
END
OLDPATH=$PATH
PATH=/tmp
. onpath.sh p1
PATH=$OLDPATH
cd /home/oracle
. onpath.sh 2>&1
echo "st=$?"
shopt -u sourcepath
PATH=/tmp
. onpath.sh 2>&1
echo "nosourcepath st=$?"
PATH=$OLDPATH
shopt -s sourcepath
source -p /tmp onpath.sh p2
echo "st=$?"
source -p /nowhere onpath.sh 2>&1
echo "st=$?"
