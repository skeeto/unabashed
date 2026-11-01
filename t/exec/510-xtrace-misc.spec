== stdout 30
ab
fmt
y
piped
a b c"d
p 1 p2

== stderr 207
++ echo a
+ x=a
+ y=b
+ echo ab
+ : 3
+ printf '%s\n' fmt
+ declare -x d=1
+ local_outside=1
+ unset x
+ [[ -n b ]]
+ echo y
+ true
+ echo piped
+ echo 'a b' 'c"d'
+ set -- 'p 1' p2
+ echo 'p 1' p2
+ set +x

== merged 237
++ echo a
+ x=a
+ y=b
+ echo ab
ab
+ : 3
+ printf '%s\n' fmt
fmt
+ declare -x d=1
+ local_outside=1
+ unset x
+ [[ -n b ]]
+ echo y
y
+ true
+ echo piped
piped
+ echo 'a b' 'c"d'
a b c"d
+ set -- 'p 1' p2
+ echo 'p 1' p2
p 1 p2
+ set +x

== status 0
