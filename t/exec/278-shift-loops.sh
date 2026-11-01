# loops consuming positional parameters
set -- a b c d e
while [ $# -gt 0 ]; do
    case $1 in
        b) shift; echo "b takes $1"; shift; continue;;
        d) break;;
    esac
    echo "arg $1"
    shift
done
echo "left: $*"
f() {
    while [ $# -gt 0 ]; do echo "f $1"; shift; done
    echo "f done $#"
}
f x y
echo "outer still: $#"
set -- 1 2 3
for x; do set -- q; echo "x=$x \$#=$#"; done
