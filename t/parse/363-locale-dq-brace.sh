f() {
    echo "${x:-$"abc"}" "${x#$"a b"}" "${x/$"p"/$"r"}" ${x:-$"u"}
}
declare -f
