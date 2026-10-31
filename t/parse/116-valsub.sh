f() {
    echo ${| REPLY=x; } "${| cmd; }"
    echo ${|
        REPLY=multi
    }
}
declare -f
