readonly r=1
echo "r=$r"
r=2
echo "after assign status $? r=$r"
declare r=3
echo "declare status $? r=$r"
readonly r=4
echo "readonly again status $? r=$r"
r+=5
echo "append status $? r=$r"
f() { local r=6; echo "local status $?"; }
f
echo "r=$r"
export r=7
echo "export status $? r=$r"
