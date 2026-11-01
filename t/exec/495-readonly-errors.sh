readonly r=1
r=2
echo "a st=$? r=$r"
r=3 echo "prefix"
echo "b st=$?"
(r=4; echo after-c); echo "st=$?"
unset r
echo "d st=$? r=$r"
declare r=5
echo "e st=$?"
export r=6
echo "f st=$?"
readonly r=7
echo "g st=$?"
for r in x y; do echo "loop $r"; done
echo "h st=$?"
f() { local r=8; echo "in f $r"; }
f
echo "i st=$?"
r+=9
echo "j st=$? r=$r"
read r <<< "input"
echo "k st=$? r=$r"
: ${r:=z}
echo end
