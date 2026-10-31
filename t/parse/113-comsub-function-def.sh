f() {
    x=$(g() { echo g; }; g)
    y=$(function h { :; })
}
declare -f
