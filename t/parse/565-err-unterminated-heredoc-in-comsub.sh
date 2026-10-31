echo start
f() {
    echo $(cat <<E
x
)
}
echo notreached
