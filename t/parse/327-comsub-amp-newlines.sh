f() {
    a=$(x &
y &
)
    b=$(x &

y)
    c=$(x && y &
z)
    d=$(x; y &
z)
}
declare -f
