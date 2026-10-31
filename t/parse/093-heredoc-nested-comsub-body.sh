f() {
    cat <<EOF
$(echo "a)b")
${x//\}/y}
EOF
}
declare -f
