f() {
    x=$(
        # comment with ) paren
        echo a # trailing )
    )
    y=$(echo '#' "#" a#b)
}
declare -f
