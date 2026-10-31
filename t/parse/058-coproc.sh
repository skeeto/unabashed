f() {
    coproc cmd arg
    coproc NAME { cmd; }
    coproc ( cmd )
    coproc NAME2 ( a; b )
    coproc { a; b; }
    coproc while read l; do echo $l; done
}
declare -f
