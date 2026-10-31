f() {
    a="x y" b='z' c=$'q' d=$(cmd) e=`cmd` g=${h:-i} j=$((1)) k=~
    echo a=b ./x=y
    declare -A m=([a]=1 ["b c"]=2)
    local -n ref=a
    readonly r=1
    typeset t
    x+=(more) y[2]+=z
}
declare -f
