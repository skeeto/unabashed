# return with bad arguments inside a function
f() { return abc; echo "after return abc"; }
f
echo "abc: $?"
g() { return -1; }
g
echo "-1: $?"
h() { return 256; }
h
echo "256: $?"
k() { return 300; }
k
echo "300: $?"
m() { return ' 5 '; }
m
echo "spaces: $?"
n() { return -- 6; }
n
echo "dashdash: $?"
