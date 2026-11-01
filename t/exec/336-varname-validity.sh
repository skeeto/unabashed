declare _ok=1 _=2 A1=3
echo "$_ok $A1"
declare 'x-y=1'
echo "status $?"
readonly 'r-r=1'
echo "status $?"
export 'e.e'
echo "status $?"
f() { local '%'; echo "local status $?"; }
f
declare -p 'p q'
echo "status $?"
