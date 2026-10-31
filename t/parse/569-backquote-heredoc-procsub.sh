f() {
    echo `cat <(echo a) <<E
x
E`
    echo `cat <<'E'
$y \` \\
E
`
    echo `echo \`cat <<E
z
E\``
    echo "`cat <<E
w
E`"
}
declare -f
