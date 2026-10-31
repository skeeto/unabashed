shopt -s extglob
f() {
    echo @(a|*(b|+(c))) x!(y)z ?(\)) @(a\|b) "@("x")" @(a'|'b) +($(echo d))
    case $x in !(*(a)|b)|@(c)) ;; esac
}
declare -f
