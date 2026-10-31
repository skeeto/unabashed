f() {
    a=$(x $(y
z) $(if p; then q; fi))
    b="$(echo "$(x;
y)")"
    c=$(echo `x;
y`)
}
declare -f
