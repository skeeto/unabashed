# Words in ${x-word} and friends: unquoted words split, quoted parts
# stay together.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show ${u-a b}
show "${u-a b}"
show ${u-"a b"}
show ${u-"a b" c}
show ${u-'a  b'}
v='1 2'
show ${u-$v}
show ${u-"$v"}
show "${u-$v}"
show ${v+x y}
show ${v:+"$v"}
show ${u-}
show ${u-''}
show ${u-x''}
show "${u-''}"
show "${u-"a b"}"
set -- p 'q r'
show ${u-"$@"}
show "${u-"$@"}"
show ${u-$@}
