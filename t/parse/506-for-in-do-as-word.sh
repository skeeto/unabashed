f() {
    for x in do; do :; done
    for x in done in esac; do :; done
    for x in { }; do :; done
}
declare -f
