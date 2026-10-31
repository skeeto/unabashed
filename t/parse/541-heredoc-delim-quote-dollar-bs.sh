f() {
    cat <<'E"X'
x $y
E"X
    cat <<"a$b"
y $z
a$b
    cat <<\E\N\D
z
END
    cat <<E\$
w $w
E$
    cat <<E'$'x
v
E$x
    cat <<$X
u $u
$X
    cat <<"$(x)"
t
$(x)
    cat <<a\\b
s
a\b
    cat <<"a\"b"
r
a"b
    cat <<'a\b'
q
a\b
}
declare -f
