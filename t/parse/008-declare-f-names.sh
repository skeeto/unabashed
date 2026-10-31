f() { echo f; }
g() { echo g; }
h() { echo h; }
declare -f h f
echo between
declare -f g
