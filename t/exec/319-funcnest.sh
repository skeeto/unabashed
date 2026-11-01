FUNCNEST=3
f() { echo "level $1"; f $(($1 + 1)); }
f 1
echo "status $?"
echo after
FUNCNEST=0
g() { if [ $1 -lt 5 ]; then g $(($1 + 1)); else echo "reached $1"; fi; }
g 1
