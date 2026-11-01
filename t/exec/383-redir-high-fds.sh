# explicit descriptor numbers above 9
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
exec 12>/tmp/f
echo twelve >&12
exec 12>&-
show /tmp/f
echo nine 9>/tmp/g >&9
show /tmp/g
exec 99>/tmp/h
echo ninety-nine >&99
exec 99>&-
show /tmp/h
echo big >&1023
echo status $?
