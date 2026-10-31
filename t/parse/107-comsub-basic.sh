f() {
    echo $(cmd) $(cmd arg | other) $( spaced ) $(a; b) $(a && b || c)
    echo "$(cmd)" "x$(cmd)y" $(
        multi
        line
    )
}
declare -f
