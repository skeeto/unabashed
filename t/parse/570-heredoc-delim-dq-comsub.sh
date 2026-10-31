f() {
    cat <<"$(echo ")")"
y
$(echo ))
}
declare -f
f() {
    cat <<"$(echo "a b")"
y
$(echo a b)
    cat <<$(echo 'x y')
z
$(echo 'x y')
    cat <<${x:-"q r"}
w
${x:-"q r"}
    cat <<"${x:-"q r"}"
v
${x:-q r}
    cat <<a"$(echo "b")"c
u
a$(echo b)c
}
declare -f
