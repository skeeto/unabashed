f() {
    if a; then b; fi
    if a; then b; else c; fi
    if a; then b; elif c; then d; elif e; then g; else h; fi
}
declare -f
