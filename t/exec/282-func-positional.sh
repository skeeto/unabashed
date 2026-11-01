set -- outer1 outer2 outer3
show() { echo "n=$# all=[$*] 1=[$1] 2=[$2]"; }
show
show a "b c"
show ""
f() {
    show "$@"
    set -- x y
    show "$@"
    shift
    echo "after shift: $# $1"
    shift 5; echo "shift 5: $?"
}
f p q r
echo "outer: $# $*"
g() { echo "in g: ${10} ${11} $#"; }
g 1 2 3 4 5 6 7 8 9 ten eleven
h() { shift; echo "h: $*"; }
h
echo "status $?"
echo "outer still: $1 $2 $3"
