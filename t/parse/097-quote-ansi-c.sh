f() {
    echo $'tab\there' $'nl\n' $'quote\'s' $'\x41\101é' $'\e[0m' $'\cA'
    echo $"translated $x" $''
}
declare -f
