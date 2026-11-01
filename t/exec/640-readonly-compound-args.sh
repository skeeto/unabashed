# A literal name=(...) given to declare, readonly or export when name is
# readonly: bash declares the name first, reporting a failure under the
# stale command name (the calling function's, at the start of its body)
# and abandoning the command, unless it declares a local
readonly -a a=(1)
readonly -a a=(2)
declare -a a=(2)
readonly a=(2)
declare a=(2)
readonly -a 'a=(4)'
declare -a 'a=(4)'
readonly 'a=(4)'
f() { declare -a a=(2); }; f
f() { :; declare -a a=(2); }; f
f() { readonly a=(2); }; f
f() { readonly -a a=(2); echo not reached; }; f; echo $?
f() { export -a a=(2); }; f
f() { local -a a=(2); }; f
f() { declare a=(2) b=(1); }; f; declare -p b 2>&1
c=(x); readonly 'c=(3)'; declare -p c
e=(x); export 'e=(3)'; declare -p e
d=(x); declare 'd=(3)'; declare -p d
