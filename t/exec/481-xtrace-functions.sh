f() { echo "in f: $1"; return 2; }
set -x
f 'a b'
echo "st=$?"
g() { f x; }
g
h() {
  local v=1
  echo $v
}
h > /dev/null
echo hi > /tmp/out 2>&1
echo red 2>/dev/null
set +x
echo done
set -o xtrace
: via-o
set +o xtrace
echo off
