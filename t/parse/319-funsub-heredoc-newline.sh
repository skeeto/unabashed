f() {
    y=${ cat <<A
a
A
}
    z=${ cat <<B
b
B
echo more
}
}
declare -f
