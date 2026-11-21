# A temporary binding of a readonly name fails and binds nothing, so
# nothing is unbound after the command either: an outer temporary
# binding of the name (a function's) stays, nor does a program see
# the value in its environment
g() { echo "g: X=$X"; }
f() {
    readonly X=2
    X=3 g;          echo "after g: X=${X-unset}"
    X=4 true;       echo "after true: X=${X-unset}"
    X=5 cat </dev/null; echo "after cat: X=${X-unset}"
    X=6 printenv X; echo "printenv: $?"
}
X=1 f
echo "top: X=${X-unset}"
k() { readonly Z=2; A=a Z=3 B=b g; echo "k: A=${A-unset} Z=$Z B=${B-unset}"; }
Z=1 A=outer k
echo "top: Z=${Z-unset} A=${A-unset}"
