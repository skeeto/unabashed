f() {
    [[ $x == *.txt ]]
    [[ $x == "*.txt" ]]
    [[ $x != a?c ]]
    [[ $x == [!a-z]* ]]
    [[ $x = $pat ]]
    [[ $x == "$pat" ]]
}
declare -f
