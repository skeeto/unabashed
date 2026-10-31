f() {
    x=$(cat <<A && echo y || cat <<B
a
A
b
B
)
    z=$(cat <<A &
a
A
echo bg)
}
declare -f
