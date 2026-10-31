shopt -s extglob
f() {
    echo ${x##+([[:space:]])} ${x%%*( )} ${x//@(a|b)/c}
}
declare -f
