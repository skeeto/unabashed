f() {
    coproc X if a; then b; fi
    coproc for i in 1; do :; done
    coproc [[ x ]]
    coproc (( x ))
    coproc N (( x ))
    coproc N [[ y ]]
    coproc case x in x) ;; esac
}
declare -f
