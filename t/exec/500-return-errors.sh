return
echo "a st=$?"
return 3
echo "b st=$?"
f() { return abc; echo after; }
f
echo "c st=$?"
h() { return 300; }
h
echo "e st=$?"
k() { return -2; }
k
echo "f st=$?"
m() { false; return; }
m
echo "g st=$?"
n() { (return 4); echo "sub st=$?"; }
n
x=$(return 5)
echo "h st=$?"
p() { (exit 6); return; }
p
echo "i st=$?"
q() { return -- 7; }
q
echo "j st=$?"
echo end
