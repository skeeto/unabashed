f() {
    if a && b; c || d; then e; g; fi
    if ! a | b; then :; fi
    if { a; b; }; then (c); fi
}
declare -f
