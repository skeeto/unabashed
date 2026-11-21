== stdout 160
rc=0
rc=0
rc=0
rc=1
f rc=0
r rc=1
t rc=1
s rc=0
a b
alive
a b
p1q
d
<a,b><c>
p q
a b
a b
declare -a b=()
BASH_SUBSHELL back
a b
FUNCNAME back
a b
a[1] back
a b

== stderr 0

== merged 160
rc=0
rc=0
rc=0
rc=1
f rc=0
r rc=1
t rc=1
s rc=0
a b
alive
a b
p1q
d
<a,b><c>
p q
a b
a b
declare -a b=()
BASH_SUBSHELL back
a b
FUNCNAME back
a b
a[1] back
a b

== status 0
