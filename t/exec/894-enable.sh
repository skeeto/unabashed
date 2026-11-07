# enable: listings, disabling and enabling, and what a disabled
# builtin leaves (functions, files, builtin, type, command)
enable -ps
enable -s | head -3
enable -n echo test; echo "status $?"
enable -n
enable -an | grep -e -n
type echo; echo "status $?"
echo hi; echo "status $?"
builtin echo hi; echo "status $?"
command -v test; echo "status $?"
enable -a | grep -E 'echo|test'
( enable echo; echo sub ); echo "status $?"
enable echo; echo back
enable -p | grep -c 'enable -n'
enable nosuch echo other; echo "status $?"
enable -d notbuiltin; echo "status $?"
enable -d test; echo "status $?"
enable -x; echo "status $?"
enable -f; echo "status $?"
enable -n enable; enable test; echo "status $?"
builtin enable test 2>&1 | head -1
