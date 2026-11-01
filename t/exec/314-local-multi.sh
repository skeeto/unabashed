f() {
    local a=1 b c=3
    echo "a=$a b=[${b-unset}] c=$c"
    local "d=x y" e="p q"
    echo "d=$d e=$e"
    local f=$a$c g="$a $c"
    echo "f=$f g=$g"
    local w="one two"
    local v=$w
    echo "v=$v"
    local -i i1=2 i2=i1*3
    echo "i1=$i1 i2=$i2"
    local a=redo
    echo "a=$a"
}
f
echo "a=[${a-unset}] d=[${d-unset}]"
