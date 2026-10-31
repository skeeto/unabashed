f() {
    cat <(cat <(echo deep))
    cat <(echo $(cmd)) "$(cat <(x))"
    cat <(case x in x) echo y ;; esac)
}
declare -f
