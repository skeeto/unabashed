set -v
echo one
x=2; echo $x
f() {
  echo fn
}
f
set +v
echo quiet
