# POSIX mode and its neighbours. An unquoted $@ in an operator's word,
# with IFS set and not starting with a space, joins the parameters with
# spaces as if quoted (POSIX interp 888). ${!?} and ${!#} are $! in
# POSIX mode. An arithmetic expansion error is fatal there, status 127.
# A substring of an unset parameter expands nothing, not even its
# offset. An expansion error in a redirection, even ${x?}, only fails
# the redirection, with the status the shell would have exited with.
# extquote decides $'...' in a double-quoted ${...}; here-documents
# translate it only in patterns. In POSIX mode . never reads a file
# from the working directory unless the name has a slash.
p() { printf '<%s>' "$@"; echo; }
set -- ' a:b' 'c d' 'e '
IFS=:
p ${v-$@}; p ${v-x$@y}; p ${v-$*}; p ${v:-$@}; p ${v-"$@"}; p ${1+$@}
set -- '*' 'a b'
p ${v-$@}; p ${v:-$@}x; x='* a b'; p ${x#$@}; p ${x/$@/R}; p ${x/\*/$@}
a=(1 '2:3' '')
p ${v-${a[@]}}; p ${v-${a[*]}}
set --
p ${v-$@}; p ${v-x$@}; p ${v-$@ }
IFS=
set -- 'a b' c
p ${v-$@}
unset IFS
p ${v-$@}
IFS=' :'
p ${v-$@}
unset IFS

bash -c 'echo ${!?}' 2>&1; echo st $?
bash -o posix -c 'echo ${!?}' 2>&1; echo st $?
bash -o posix -c 'echo ${!#}' 2>&1; echo st $?

for o in +o -o; do
    bash $o posix -c 'echo $(( x+ ))
echo after' 2>&1; echo st $?
    bash $o posix -c 'x=$(( 1/0 )); echo after' 2>&1; echo st $?
    bash $o posix -c 'f() { echo $[ x+ ]; }; f; echo after' 2>&1; echo st $?
    bash $o posix -c 'eval "echo \$(( x+ ))"; echo after' 2>&1; echo st $?
    bash $o posix -c '(( x+ )); let "x+"; echo after' 2>&1; echo st $?
    bash $o posix -c 'echo $(echo $(( x+ )); echo insub); echo after' 2>&1; echo st $?
done

bash -c 'echo "<${x:$(( x+ ))}>"; echo "<${x:$(echo hi >&2)}>"' 2>&1
bash -c 'echo "<${x:1:$(( x+ ))}>" "<${a[@]:$(( x+ ))}>" "<${a[1]:$(( x+ ))}>"' 2>&1
bash -c 'echo "<${2:$(( x+ ))}>"; declare -A A; echo "<${A[k]:$(( x+ ))}>"' 2>&1
bash -c 'x=; echo "<${x:$(( x+ ))}>"; echo after' 2>&1; echo st $?
bash -c 'set --; echo "<${@:$(( x+ ))}>"; echo after' 2>&1; echo st $?
bash -c 'x=abc; echo "<${x:x+}>"; echo after' 2>&1; echo st $?

bash -uc 'cat <<E
$nope
E
echo st $?' 2>&1
bash -o posix -c 'cat <<E
${x:?boom} $((1+))
E
echo st $?' 2>&1
bash -c 'declare -i n; cat <<E
$((n=1+))
E
echo st $?' 2>&1
bash -c 'readonly r; cat <<E
${r:=v}
E
echo st $?' 2>&1
bash -o posix -c 'readonly r; cat <<E
${r:=v}
E
echo st $?' 2>&1
bash -c 'cat <<<${x:?bad}; echo st $?' 2>&1
bash -c 'cat 3<${x:?bad}; echo st $?' 2>&1
bash -c 'cat <<E >out
${x:?bad}
E
echo st $?; test -e out || echo no out' 2>&1
bash -c 'cat <<E || echo or
${y:?bad}
E
echo st $?' 2>&1
for i in 1 2; do cat <<E
$i ${$'x1'}
E
echo st $?; done 2>&1
{ cat <<E; echo in $?; } 2>&1; echo out $?
${a[1+]}
E

x=notOK x1=not y=tab
echo "1 ${x#${$'x1'%$'t'}}"
echo "2 ${$'x\061'} ${y/a/$'\t'}| ${v-$'a\tb'}| ${v-'a'}| ${#$'x1'}"
f() { echo "3 ${$'x1'%$'t'}"; }; declare -f f
echo "4 ${x#${$'x1'}} ${v-${$'x1'}} ${v-${x#$'no'}} ${v-${v-$'a\tb'}}"
echo 5 ${$'x1'} 2>&1
shopt -u extquote
echo "6 ${x#$'no'} ${v-$'a\tb'}|"
echo "7 ${$'x1'}" 2>&1
shopt -s extquote
cat <<E
8 ${x#${$'x1'%$'t'}} ${x#$'no'} ${v-$'a\tb'}| ${x#${$'x1'}}
9 ${x/${$'x1'}/$'\t'} ${v-${x#$'no'}}
E
cat <<E 2>&1
10 ${v-${$'x1'}}
E
cat <<E 2>&1
11 ${$'x1'}
E
cat <<E 2>&1
12 ${x#${'x1'%$'t'}}
E
echo "13 ${x#${'x1'%$'t'}}" 2>&1

echo 'echo sourced $1' > sf
bash -c 'PATH=/nonexist; . sf; echo st $?' 2>&1
bash -o posix -c 'PATH=/nonexist; . sf; echo st $?' 2>&1; echo st $?
bash -o posix -c 'PATH=/nonexist; source sf a; echo st $?' 2>&1; echo st $?
bash -o posix -c 'PATH=/nonexist; shopt -u sourcepath; . sf' 2>&1; echo st $?
bash -o posix -c 'PATH=/nonexist; . ./sf; command . sf; echo st $?' 2>&1
bash -o posix -c 'PATH=/nonexist; . -p "$PWD" sf x' 2>&1
