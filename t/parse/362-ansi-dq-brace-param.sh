# nofixedpoint: bash's own output does not reparse
f() {
    echo "${x:$'1'}" "${a[$'k']}" ${a[$'k']} "${x-$'it\'s'}"
    echo "$[ $'1' ]" "$(echo "${x:-$'\t'}")" "${x:-$(echo $'\t')}"
}
declare -f
