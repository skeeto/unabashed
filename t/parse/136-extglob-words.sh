shopt -s extglob
f() {
    echo @(a|b) !(*.o) +(x) *(y) ?(z) pre@(a|b)post
    echo !(@(a|b)|c) "@(not)" \@\(no\)
    rm -- !(keep)
}
declare -f
