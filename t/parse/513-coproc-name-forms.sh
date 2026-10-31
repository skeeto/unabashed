f() {
    coproc foo bar
    coproc N ((x))
    coproc [[ -n a ]]
    coproc N [[ -n a ]]
    coproc N if a; then b; fi
    coproc a=1 cmd
    coproc >out cmd
}
declare -f
