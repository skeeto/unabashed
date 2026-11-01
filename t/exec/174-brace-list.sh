# Brace expansion of comma lists, with preamble and postscript, nested.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show {a,b,c}
show x{a,b}y
show {a,b}{1,2}
show {a,{b,c},d}
show x{,a}
show {,}
show {a,}b
show {a,b{1,2}c}d
show {{a,b},{c,d}}
show a{b,c}d e{f,g}
show {a}
show {}
show { a,b }
show {a,b
show a,b}
show {a,b}}
show {{a,b}
show '{a,b}'
show {'a,b',c}
show {a\,b,c}
show "{a,b}"
show {"a b",c}
show \{a,b}
