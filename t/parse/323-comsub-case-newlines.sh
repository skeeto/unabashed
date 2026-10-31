f() {
    x=$(case a in
a) b
c;;
d)
e
;;
esac)
    y=$(case a in a) ;; esac; echo z)
}
declare -f
