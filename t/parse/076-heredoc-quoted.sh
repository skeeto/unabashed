f() {
    cat <<'EOF'
$x `y` $(z)
EOF
    cat <<"EOF"
$x \$y
EOF
    cat <<E\OF
$x
EOF
    cat <<'E'"F"
literal $x
EF
}
declare -f
