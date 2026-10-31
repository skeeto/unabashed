# nofixedpoint: bash's own output does not reparse
f() {
    cat <<'E' <<F; a=(1
E
F
2)


    echo next
}
declare -f
