f() {
    echo $( # comment with ) paren
    echo a # it's "quoted" (
    )
    echo $(echo b # )
    )
    echo $(
        # ) ( ' " `
        echo c
    )
    echo "$( echo d # )" '
    )"
    echo $(echo e#notcomment)
    echo $(echo f;#comment )
    )
}
declare -f
