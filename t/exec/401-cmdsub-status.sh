# Exit status of command substitution propagates via assignments
x=$(false)
echo "assign false: $?"
x=$(true)
echo "assign true: $?"
x=$(exit 7)
echo "assign exit 7: $?"
x=$(exit 3) y=$(exit 5)
echo "two assigns: $?"
x=$(exit 4) true
echo "prefix on true: $?"
x=$(exit 4) false
echo "prefix on false: $?"
echo $(exit 9)
echo "echo cmdsub: $?"
: $(false)
echo "colon: $?"
x=`exit 6`
echo "backquote: $?"
export v=$(exit 2)
echo "export: $?"
local_test() { local l=$(exit 8); echo "local: $?"; }
local_test
readonly r=$(exit 11)
echo "readonly: $?"
