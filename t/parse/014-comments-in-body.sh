f() {
    # leading comment
    echo a # trailing comment
    # between
    echo b#notcomment c #comment
    echo '#quoted' "#dq" \#esc
}
declare -f
