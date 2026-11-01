# The five extglob operators in case patterns, alone, nested and with
# alternatives, including empty alternatives and literal parentheses.
shopt -s extglob
m() {
    local p=$1; shift
    printf '%-14s' "$p"
    for s; do
        case $s in
        $p) printf ' %s' "${s:-''}" ;;
        esac
    done
    echo
}
set -- '' a b ab ba aa aab abab abc c x.c x.h .c xc
m '?(a)'         "$@"
m '*(a)'         "$@"
m '+(a)'         "$@"
m '@(a|b)'       "$@"
m '!(a)'         "$@"
m '+(a|b)'       "$@"
m '*(ab)'        "$@"
m '+(a)b'        "$@"
m 'a*(b)'        "$@"
m '!(a*)'        "$@"
m '*.!(c)'       "$@"
m '*.@(c|h)'     "$@"
m '@(|a)'        "$@"
m '?(|b)a'       "$@"
m '+(?(a)b)'     "$@"
m '*(+(a)b)'     "$@"
m '!(!(a))'      "$@"
m '@(a|!(b))'    "$@"
m '!(+(a)|b)'    "$@"
m '@(x.@(c|h))'  "$@"
m '+(*.c|b)'     "$@"
m 'a!(b)*'       "$@"
case 'a(b)' in @(a\(b\))) echo literal parens ;; esac
case '|' in @(\|)) echo literal bar ;; esac
case 'ab' in @(a|b)@(a|b)) echo two groups ;; esac
x='+(a)'
case aaa in $x) echo from variable ;; esac
case aaa in "$x") echo quoted ;; *) echo quoted no match ;; esac
