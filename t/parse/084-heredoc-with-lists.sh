f() {
    cat <<EOF && echo ok || echo no
body
EOF
    cat <<EOF &
bg
EOF
    cat <<EOF > out 2>&1
redir
EOF
}
declare -f
