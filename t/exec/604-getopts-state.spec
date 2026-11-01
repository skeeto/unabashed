== stdout 108
a 1
b 1
a 1
opt: x 1
opt: y 1
opt: a 1
opt: b 1
opt: c 2
opt: z 2
1 ? 2
1
2
1 2
0 ? unset
0 x=x
1 unset
2 2

== stderr 227
bash: line 18: getopts: -a: invalid option
getopts: usage: getopts optstring name [arg ...]
bash: line 20: getopts: `opt-var': not a valid identifier
bash: line 25: OPTARG: readonly variable
bash: line 27: V: readonly variable

== merged 335
a 1
b 1
a 1
opt: x 1
opt: y 1
opt: a 1
opt: b 1
opt: c 2
opt: z 2
1 ? 2
1
bash: line 18: getopts: -a: invalid option
getopts: usage: getopts optstring name [arg ...]
2
bash: line 20: getopts: `opt-var': not a valid identifier
1 2
0 ? unset
bash: line 25: OPTARG: readonly variable
0 x=x
1 unset
bash: line 27: V: readonly variable
2 2

== status 0
