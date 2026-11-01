local x=1
echo "status $?"
local
echo "status $?"
f() {
    local 1bad=2
    echo "bad status $?"
    local good=1 'b c'=2 ok=3
    echo "mixed status $? good=$good ok=$ok"
    local -Z
    echo "option status $?"
}
f
