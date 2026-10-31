f() {
    echo $(echo ')') $(echo ")") $(echo \)) $(echo "$(echo "(")")
    echo $(echo 'it'\''s') "$(echo "a b")"
}
declare -f
