f() {
    echo a\\
    echo b \\\
c "d\\
e" 'f\\
g'
}
declare -f
