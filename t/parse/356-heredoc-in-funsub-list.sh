f() {
    x=${ cat <<A | wc; }
a
A
    y=${ cat <<B && echo ok
b
B
}
}
declare -f
