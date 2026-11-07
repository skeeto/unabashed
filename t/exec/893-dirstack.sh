# pushd, popd and dirs: the stack, rotation, -n, indexes, errors,
# ~N, DIRSTACK, and subshells keeping their changes
mkdir -p a b c 'd e'
top=$PWD
dirs; dirs -v; dirs -p; dirs +0; dirs -0; echo "status $?"
pushd; echo "status $?"
popd; echo "status $?"
dirs +1; echo "status $?"
pushd a; echo "status $?"; pwd
pushd ../b; pwd
pushd "../d e"; pwd
dirs; dirs -l; dirs -v; dirs -p; dirs -v -p
dirs +0; dirs +1; dirs -0; dirs -1; dirs +3; dirs -3; dirs +4; echo "status $?"
dirs -4; echo "status $?"
dirs +x; echo "status $?"
dirs -lv; echo "status $?"
dirs foo; echo "status $?"
dirs -- +1; dirs +1 +2
echo "${DIRSTACK[@]} ${#DIRSTACK[@]}"
echo ~0 ~1 ~+1 ~-0 ~-1 ~3 ~4 ~+x
pushd; pwd
pushd +2; pwd
pushd -1; pwd
pushd +0; pwd
pushd -0; pwd
pushd +9; echo "status $?"
pushd -9; echo "status $?"
pushd +q; echo "status $?"
pushd -z; echo "status $?"
pushd nosuch; echo "status $?"; dirs
pushd a b; echo "status $?"
pushd -n "$top/c"; echo "status $?"; pwd
pushd -n +1; echo "status $?"; dirs; pwd
pushd -n; echo "status $?"
( pushd "$top/c" >/dev/null; dirs ); dirs
echo "$(popd >/dev/null; dirs)"; dirs
f() { pushd "$top" >/dev/null; }; f; dirs
DIRSTACK[1]=/elsewhere; dirs
DIRSTACK[0]=/zero; DIRSTACK[99]=/far; dirs
popd -n; echo "status $?"; pwd
popd +1; popd -1; echo "status $?"
popd +7; echo "status $?"
popd -7; echo "status $?"
popd x; echo "status $?"
popd -; echo "status $?"
popd +a; echo "status $?"
popd; pwd
popd; popd; echo "status $?"
popd; echo "status $?"
popd +0; echo "status $?"
popd -n; echo "status $?"
dirs -c; dirs
cd "$top"; pushd a >/dev/null; pushd "$top/b" >/dev/null; dirs -c; dirs; pwd
cd "$top"; pushd - ; pwd
HOME=$top; cd "$top"; pushd a; dirs; dirs -l; HOME=/; dirs
