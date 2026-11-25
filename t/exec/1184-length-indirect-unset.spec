== stdout 1025
-- echo ${#u[a b]}
0
status 0
-- echo ${#u[1/0]} ${#u[-1]} ${#u[$(echo ran >&2)]}
0 0 0
status 0
-- declare -a e; echo ${#e[a b]}
0
status 0
-- declare -A m; echo ${#m[a b]}
0
status 0
-- u=1; echo ${#u[a b]}
bash: line 1: a b: arithmetic syntax error in expression (error token is "b")
status 1
-- u=1; echo ${#u[-5]}
bash: line 1: [-5]: bad array subscript
status 1
-- a=(1); echo ${#a[1/0]}
bash: line 1: 1/0: division by 0 (error token is "0")
status 1
-- set -u; echo ${#u[1/0]}
bash: line 1: u[1/0]: unbound variable
status 127
-- export x; echo "<${!x}>"
<>
status 0
-- declare x; echo "<${!x}>"
<>
status 0
-- f() { local x; echo "<${!x}>"; }; f
<>
status 0
-- echo "<${!x}>"
bash: line 1: x: invalid indirect expansion
status 1
-- set -u; declare x; echo "<${!x}>"
bash: line 1: !x: unbound variable
status 127
-- declare arr[]+=a; declare arr[]=a
bash: line 1: arr[]: bad array subscript
bash: line 1: arr[]: bad array subscript
status 1
-- declare +x u=1 arr[]+=a
bash: line 1: arr[]: bad array subscript
status 1

== stderr 0

== merged 1025
-- echo ${#u[a b]}
0
status 0
-- echo ${#u[1/0]} ${#u[-1]} ${#u[$(echo ran >&2)]}
0 0 0
status 0
-- declare -a e; echo ${#e[a b]}
0
status 0
-- declare -A m; echo ${#m[a b]}
0
status 0
-- u=1; echo ${#u[a b]}
bash: line 1: a b: arithmetic syntax error in expression (error token is "b")
status 1
-- u=1; echo ${#u[-5]}
bash: line 1: [-5]: bad array subscript
status 1
-- a=(1); echo ${#a[1/0]}
bash: line 1: 1/0: division by 0 (error token is "0")
status 1
-- set -u; echo ${#u[1/0]}
bash: line 1: u[1/0]: unbound variable
status 127
-- export x; echo "<${!x}>"
<>
status 0
-- declare x; echo "<${!x}>"
<>
status 0
-- f() { local x; echo "<${!x}>"; }; f
<>
status 0
-- echo "<${!x}>"
bash: line 1: x: invalid indirect expansion
status 1
-- set -u; declare x; echo "<${!x}>"
bash: line 1: !x: unbound variable
status 127
-- declare arr[]+=a; declare arr[]=a
bash: line 1: arr[]: bad array subscript
bash: line 1: arr[]: bad array subscript
status 1
-- declare +x u=1 arr[]+=a
bash: line 1: arr[]: bad array subscript
status 1

== status 0
