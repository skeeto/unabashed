f() {
    echo ${a[0]} ${a[i+1]} ${a[@]} ${a[*]} "${a[@]}" ${#a[@]} ${#a[1]}
    echo ${a[@]:1:2} ${a[@]/x/y} ${a[@]#p} ${m[key]} ${m["k k"]} ${m[$k]}
    a=([0]=x [5]=y) b=() c=(
        multi
        line
    )
}
declare -f
