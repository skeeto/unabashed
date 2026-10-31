f() {
    for x do echo $x; done
    select y do echo $y; done
    for z	do :; done
}
declare -f
