f() { echo $#; printf '<%s>' "$@"; echo; }
x=x e= v=" b "
f ${x:+"$e" "$e"""}
f ${x:+'' ''}
f ${x:+"" b}
f ${x:+''$e $e''}
f ""$v
f ''$v''
f "${x:+}"$v
f "$e"$v
set -- '' ''
f ${x+"$@" "$@"}
f "$*"$v
f "$@"$v
a=('' x '')
f "${a[@]}"$v
IFS=:
v=":b:"
f ""$v
f "$@"$v""
unset IFS
f ''"" $e""''
