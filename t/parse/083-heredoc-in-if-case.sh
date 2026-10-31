f() {
    if cat <<EOF; then
cond
EOF
        cat <<EOF
then
EOF
    fi
    case $1 in
        a) cat <<EOF ;;
in case
EOF
    esac
}
declare -f
