# diagnostics inside multi-line compound commands cite their own line
for i in 1; do
    echo "body"
    break 0
done
while :; do
    if true; then
        continue -2
    fi
done
case x in
    x)
        break
        ;;
esac
f() {
    for i in 1; do
        return abc
    done
}
f
until false; do
    { break 1 2 3 4; } 2>&1
    echo "after"
    break
done
