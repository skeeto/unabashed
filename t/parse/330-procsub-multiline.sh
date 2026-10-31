f() {
    diff <(x
y) >(if a; then b; fi)
    cat <(case q in q) r;; esac) <(while a; do b; done)
    cat <(cat <<A
p
A
)
}
declare -f
