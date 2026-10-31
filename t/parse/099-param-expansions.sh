f() {
    echo $x ${x} ${x:-d} ${x-d} ${x:=d} ${x=d} ${x:?msg} ${x?} ${x:+alt} ${x+alt}
    echo ${#x} ${x#p} ${x##p} ${x%s} ${x%%s} ${x/a/b} ${x//a/b} ${x/#a/b} ${x/%a/b} ${x/a}
    echo ${x:1} ${x:1:2} ${x: -1} ${x:(-2):1} ${x^} ${x^^} ${x,} ${x,,} ${x~} ${x~~}
    echo ${!x} ${!pre*} ${!pre@} ${!arr[@]} ${!arr[*]}
    echo ${x@Q} ${x@E} ${x@P} ${x@A} ${x@a} ${x@U} ${x@u} ${x@L} ${x@K} ${x@k}
}
declare -f
