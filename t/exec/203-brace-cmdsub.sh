# Brace expansion treats $(...), `...` and ${...} as opaque, even when
# they contain commas or braces.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show {a,$(echo x,y)}
show {a,`echo p,q`}
show {a,${u-b,c}}
show {a,${u-{b,c}}}
show {a,"${u-1,2}"}
show {a,$((1,2))}
show x{a,$(echo '{1,2}')}
