f() {
    cat <<A; cat <<B
first
A
second
B
    a <<X 3<<Y
x body
X
y body
Y
}
declare -f
