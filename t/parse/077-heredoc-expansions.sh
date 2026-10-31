f() {
    cat <<EOF
var $x ${y:-z} $((1+2))
cmd $(echo a) `echo b`
esc \$x \`y\` \\ \"q\" 'single'
EOF
}
declare -f
