# exit from deep inside loops and functions ends the script
f() {
    for i in 1 2 3; do
        while :; do
            case $i in 2) echo "exiting at $i"; exit 6;; esac
            echo "f $i"
            break
        done
    done
}
for w in a b; do
    f
    echo never
done
echo never
