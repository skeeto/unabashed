# bash assigns a declaration builtin's literal name=(...) while the
# command's words expand: the name is declared alone, with only the
# options that change how values are stored and its scope, then the
# array assigned, and the builtin gets the name. So the array is in
# place before a later word expands, before the temporary bindings and
# the redirections, whether the builtin, a function or nothing runs,
# and the builtin's own options (-r, +a, -n) apply afterwards.
declare fe=(1) >/nonexistent/x
echo "a $? ${fe[*]}"
set -x
declare fe2=(1 2) >/nonexistent/x
X=1 declare fe3=($X) 2>/nonexistent/x
set +x
declare -p fe2 fe3
declare z1=(x) z2=$(declare -p z1); echo "$z2"
declare a1=($(echo one >&2)) b1=$(echo two >&2)
declare -r ro3=(a) ro3=(b); echo "b $?"; declare -p ro3
declare +a ia2=(1); echo "c $?"; declare -p ia2
declare -n r3=t3; declare -n r3=(1 2); echo "d $?"; declare -p r3 t3
declare -n r4=(1); echo "e $?"; declare -p r4
local la=(1 2); echo "f $?"; declare -p la
declare -i ii=(1+1) jj=(2+2); declare -p ii jj
o=-i; declare $o n2=(2+2); declare -p n2
declare -u up=(abc) +u up2=(def); declare -p up up2
declare a[1]=(x); echo notreached
declare() { echo "fn: $*"; }
declare da=(1 2); echo "g $?"; builtin declare -p da
unset -f declare
f() { declare -p fl 2>&1; local -i fl=(3+4) >/nonexistent/x; declare -p fl; }
f
g() { local x=1; declare -G x=2 y=3; echo "in $x $y"; }
g; echo "out ${x-unset} $y"
mk() { declare -n ref=var; declare -g ref=(X); declare -p ref var 2>&1; }
declare -n ref=foo
mk; declare -p foo
echo end
