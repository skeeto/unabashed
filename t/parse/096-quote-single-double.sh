f() {
    echo 'single $x `y`' "double $x `y`" '' ""
    echo 'it'\''s' "a\"b" "a\$b" "a\\b" "a\`b" "a\b"
    echo a'b'"c"d
    echo "  spaced  " '	tab'
}
declare -f
