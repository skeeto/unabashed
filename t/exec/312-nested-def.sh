outer() {
    inner() { echo "inner defined by outer"; }
    echo "outer ran"
}
inner
echo "status $?"
outer
inner
redefine() {
    redefine() { echo "second version"; }
    echo "first version"
}
redefine
redefine
mk() { eval "gen_$1() { echo generated $1; }"; }
mk a; mk b
gen_a; gen_b
declare -F
