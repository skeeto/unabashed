f() {
    if a; then
        if b; then
            if c; then d; fi
        else
            if e; then g; else h; fi
        fi
    fi
}
declare -f
