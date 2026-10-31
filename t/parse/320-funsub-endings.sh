f() {
    a=${ x &
}
    b=${ x;
}
    c=${ { x; } }
    d=${ case y in y) x;; esac }
    e=${ ( x ) }
    g=${ x | y; }
    h=${ [[ x ]] }
    i=${ x && y
}
    j=${ x=1 }
}
declare -f
