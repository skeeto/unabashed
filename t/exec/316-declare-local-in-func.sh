x=top
f() {
    declare x=declared
    typeset y=typeset
    declare -i z=1+1
    echo "f: $x $y $z"
    g
}
g() { echo "g sees: $x $y $z"; }
f
echo "top: $x [${y-unset}] [${z-unset}]"
h() { declare -p x 2>&1; local x=l; declare -p x; }
h
k() { local; }
m() { local m1=1 m2="two words"; local -i m3=3; k; echo ---; local; }
m
