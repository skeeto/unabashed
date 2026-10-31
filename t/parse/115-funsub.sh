f() {
    echo ${ cmd; } ${ a; b; } "${ cmd; }"
    echo ${	cmd;} ${
        multi
        line
    }
    echo ${ if a; then b; fi; } ${ { x; }; }
}
declare -f
