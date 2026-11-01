x=global
f() {
    local x
    echo "x in f: [${x-unset}] [${x:-empty}]"
    declare -p x
    x=set
    echo "x now: $x"
}
f
echo "x after: $x"
g() { local u; echo "u: [${u-unset}]"; [ -v u ]; echo "-v u: $?"; }
g
h() { local x=; echo "x: [${x-unset}]"; declare -p x; }
h
k() { local v=1 w v2=2; declare -p v w v2; }
k
