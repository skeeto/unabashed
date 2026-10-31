f() {
    a[x y]=1 b["k l"]=2 c[$((1 + 2))]=3 d[ 1 ]=4
    e[a]+=5 g[`echo }`]=6 h[$(echo ])]=7
    echo i[x y]=8
    local j[p q]=9
}
declare -f
