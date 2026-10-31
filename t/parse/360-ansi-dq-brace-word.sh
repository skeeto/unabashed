# nofixedpoint: bash's own output does not reparse
f() {
    echo "${x:-$'\t'}" "${x:=$'a\nb'}" "${x+$'q\'r'}" "${x:?$'\x41'}"
}
declare -f
