# Words that are just a parameter, quoted or not, braced or not: the
# cases where one field is not the answer, and values that must outlive
# the variable
show() { printf '<%s>' "$@"; echo " $#"; }
x='a b'; show $x "$x" ${x} "${x}"
x=; show $x "$x" ${x} "${x}"
unset x; show $x "$x" ${x} "${x}"
x='*'; show "$x"; mkdir d; : >d/p; cd d; show $x; cd ..
x='a\b'; show $x "$x"
x='a(b'; show $x
IFS=: x='a:b c'; show $x "$x"; unset IFS
IFS= x='a b'; show $x; unset IFS
a=(one two); show $a "${a}"
declare -n r=a; show $r "$r"
declare -i n=6*7; show $n
show "$LINENO" $LINENO
u=old; for v in "$u" $u; do u=new$v; show "$v"; done; show "$u"
f() { local w=in; show "$w" $w; }; f
x=b; show $x$x "$x$x" $x- "$x"-
set -u; (show "$nope"); echo "status $?"; show $x
