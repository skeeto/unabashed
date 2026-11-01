# Sequence expressions {x..y[..incr]}.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show {1..5}
show {5..1}
show {-2..2}
show {1..10..3}
show {10..1..3}
show {1..10..-3}
show {01..10}
show {1..010}
show {-05..5..5}
show {a..e}
show {e..a}
show {a..j..3}
show {Z..c}
show {1..1}
show x{1..3}y
show {1..3}{a..b}
show {1..a}
show {1..}
show {..3}
show {1.5..3}
show {aa..c}
show {1..3..0}
show {a..c..0}
show {+1..3}
show {0..-2}
