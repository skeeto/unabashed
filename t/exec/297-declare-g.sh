f() {
    declare d=func-local
    declare -g g=global-from-f
    typeset t=typeset-local
    echo "in f: $d $g $t"
}
f
echo "d=[${d-unset}] g=[${g-unset}] t=[${t-unset}]"
x=top
h() { local x=local; declare -g x=global-set; echo "h sees: $x"; }
h
echo "x=$x"
k() { declare -gi gi=2+3; declare -gx gx=exp; }
k
declare -p gi gx
