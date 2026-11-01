set -x
echo a b
echo "a b" 'c'
echo ''
echo "it's"
x=1
y="a b"
z=
echo $x "$y" "$z"
: noop
true
set +x
echo untraced
