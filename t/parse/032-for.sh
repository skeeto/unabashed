f() {
    for i in a b c; do echo $i; done
    for i; do echo $i; done
    for i in; do echo never; done
    for i
    do
        echo $i
    done
    for i in a b
    do echo $i
    done
}
declare -f
