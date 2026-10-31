shopt -s extglob
f() {
    @(a) arg
    x=+(b) y
    for i in ?(a) *(b); do :; done
    a=(@(c) !(d))
    echo ${x/@(e|f)/g} >@(h)
}
declare -f
