outer() {
    inner() {
        echo inner
    }
    function deep { echo deep; }
    sub() ( echo sub )
    inner
}
declare -f outer
