echo() { builtin echo "[wrapped] $*"; }
echo hello
command echo direct
builtin echo builtin
unset -f echo
echo restored
cd() { builtin echo "fake cd $1"; }
cd /tmp
echo "pwd: $PWD"
unset -f cd
true() { return 9; }
true; echo "true func: $?"
command true; echo "command true: $?"
unset -f true
true; echo "true: $?"
