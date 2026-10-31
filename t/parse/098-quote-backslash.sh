f() {
    echo \a \$x \" \' \\ \* \  \; \& \| \< \> \( \) \{ \}
    echo a\ b c\\d
}
declare -f
