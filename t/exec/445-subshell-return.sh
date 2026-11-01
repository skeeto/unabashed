# return and exit inside subshells within functions
f() { (return 3); echo "after sub return: $?"; return 1; }
f
echo "f: $?"
g() { (exit 4); echo "g continues $?"; }
g
h() { x=$(return 5); echo "h: $?"; }
h
(return 2>&1)
echo "top sub return: $?"
k() { ( echo in-k; return 7; echo no ); }
k
echo "k: $?"
