f() {
    echo $(cat <<')'
x
)
)
    echo $(cat <<`E`
z
`E`
)
    echo "$(cat <<E
w
E
)"
    echo $(cat <<E # comment )
v
E
)
}
declare -f
