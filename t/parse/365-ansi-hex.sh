f() {
    echo $'\x41\x4a2' $'\x{41}' $'\x{000041}B' $'\x{4' $'\xg' $'\x' $'a\x{}b' $'\x{1F}c'
}
declare -f
