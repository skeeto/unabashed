f() {
    x=$(cat <<A
body
A)
    y=$(cat <<B
body
B ) more
    z=${ cat <<C
body
C }
    echo done
}
declare -f
