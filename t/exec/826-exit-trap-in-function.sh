# exit in a function runs the EXIT trap inside it; an unset local keeps
# only an export taken over from a temporary binding
exec 2>&1
bash -c 'trap "echo trap:\$FUNCNAME:\$x:\$#" EXIT; f() { local x=L; exit; }; f a b'
bash -c 'trap "echo trap:\$FUNCNAME:\$x" EXIT; f() { local x=L; g; }; g() { local x=G; exit 3; }; f; echo no'; echo st=$?
bash -c 'trap "echo trap:\$FUNCNAME:\$x" EXIT; f() { local x=L; (exit 4); echo in; }; f'
bash -c 'f() { trap "echo trap:\$FUNCNAME:\$x" EXIT; local x=L; ( exit 4 ); }; f; echo out'
bash -c 'trap "echo trap:\$FUNCNAME; exit 7" EXIT; f() { exit 2; }; f'; echo st=$?
f() { local -i v=1; unset v; declare -p v; v=2+3; declare -p v; }; f
f() { local -x v=1; unset v; declare -p v; }; f
f() { local -a v=(1); unset v; declare -p v; }; f
f() { local v=x; unset v; declare -p v; }; v=t f
v=g; f() { local v=x; unset v; declare -p v; }; f
