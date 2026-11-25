# A command substitution whose text bash prints so that it no longer
# parses (a here-document in an && list loses a separator): the error
# found as it is expanded is reported as bash's parse reports it, as
# the command substitution's, and abandons the command (# nofixedpoint:
# the printed text not parsing is the point)
f() {
    : "$(: $(echo && echo yes) <<'E' && echo yes; { y=1; }
a $x
E
)"
    echo not reached
}
f
echo "1: $?"
g() {
    cat <<X
$(: <<'E' && echo yes; { y=1; }
a
E
)
X
}
g
echo "2: $?"
h() {
    : $(: <<'E' && echo yes; { y=1; }
a
E
)
    echo not reached
}
h
echo "3: $?"
