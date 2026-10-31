f() {
    echo $(echo ${x#*}) $(echo ${#x}#) "$(echo a # )
    )" $(case x in #c
    x) # )
    ;; esac)
}
declare -f
