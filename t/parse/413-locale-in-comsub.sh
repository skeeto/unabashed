f() {
    echo "$(echo $"x" $'y')" $(echo "$"z"") $"a\$b" $"`c`" $"a'b"
}
declare -f
