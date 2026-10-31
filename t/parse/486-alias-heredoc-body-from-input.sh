shopt -s expand_aliases
alias h='cat <<EOF'
alias h2='cat <<A; echo mid'
f() {
    h
line one
EOF
    h2 x
body
A
    echo after
}
declare -f
