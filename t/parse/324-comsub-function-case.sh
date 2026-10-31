f() {
    x=$(g() { case x in esac; }; h() { case y in y) z;; esac
})
}
declare -f
