f() {
    cat <<EOF
joined \
line
EOF
    cat <<'EOF'
kept \
line
EOF
}
declare -f
