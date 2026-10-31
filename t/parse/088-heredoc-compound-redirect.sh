f() {
    { read a; read b; } <<EOF
1
2
EOF
    ( cat ) <<EOF
sub
EOF
}
declare -f
