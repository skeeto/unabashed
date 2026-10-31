f() {
    echo a ]] b [[
    x=[[ y=]]
    echo [[x ]]y
    a [[ b ]]
}
declare -f
]] a
