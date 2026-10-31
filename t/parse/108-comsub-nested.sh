f() {
    echo $(a $(b $(c)))
    echo "$(a "$(b "$(c)")")"
    echo $(echo `inner`)
}
declare -f
