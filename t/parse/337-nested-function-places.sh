f() {
    if a; then
        g() { b; }
    fi
    { h() ( c ); }
    while :; do function k { d; } done
    m() { :; } | cat
    n() { :; } && p() { :; }
}
declare -f f
