f() {
    echo $'\q\z\ ' $'\"\?' $'\\\\' $'a\\' $'\%' $'\N\k'
}
declare -f
