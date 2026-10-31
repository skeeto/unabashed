f() {
    a <<< word
    a <<< "two words"
    a <<<$x
    a 3<<< 'q'
    a <<< "$(cmd)"
}
declare -f
