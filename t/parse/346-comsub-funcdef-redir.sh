f() {
    x=$(g() { a; } > out; g)
    y=$(h() ( b ) 2>&1)
    z=$(k() { cat; } <<A
z
A
)
}
declare -f
