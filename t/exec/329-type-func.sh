f() { echo hi; }
type f
type -t f
echo "status $?"
type -t echo
type -t nosuchthing
echo "status $?"
f=var
type -t f
function g { :; }
type g
