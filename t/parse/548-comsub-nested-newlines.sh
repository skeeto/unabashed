f() {
    echo $(
        echo $(
            echo $(
                echo a
            )
            echo b
        ) $(echo c
        )
    )
    x=$( $( $(echo d) ) )
    echo "$(
        echo "$(echo "e
")"
    )"
}
declare -f
