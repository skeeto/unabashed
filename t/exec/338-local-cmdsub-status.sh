f() {
    local a=$(false)
    echo "local status: $?"
    declare b=$(false)
    echo "declare status: $?"
    c=$(false)
    echo "plain status: $?"
    local d
    d=$(exit 3)
    echo "split local status: $?"
    export e=$(false)
    echo "export status: $?"
    readonly r=$(false)
    echo "readonly status: $?"
}
f
