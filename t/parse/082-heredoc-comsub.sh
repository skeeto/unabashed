f() {
    x=$(cat <<EOF
inside
EOF
)
    y=$(cat <<'EOF'
q $x
EOF
    )
    echo "$(cat <<A
dq
A
)"
}
declare -f
