== stdout 231
[unset]
u 10
e 66
d 65
d 65
r 67
2
5
fd [echo in f]
in f
fd [echo after f]
after f
fd [g]
in g
fd [trap - DEBUG]
in r
returning from r
in r2
err in e 1
after false
err in e 1
222
exit=0
invoke 0
stat 111
BASH_TRAPSIG = USR1
func=7

== stderr 99
bash: line 10: trap: cannot specify both -p and -P
bash: line 11: declare: BASH_TRAPSIG: not found

== merged 330
[unset]
u 10
e 66
d 65
d 65
r 67
bash: line 10: trap: cannot specify both -p and -P
2
bash: line 11: declare: BASH_TRAPSIG: not found
5
fd [echo in f]
in f
fd [echo after f]
after f
fd [g]
in g
fd [trap - DEBUG]
in r
returning from r
in r2
err in e 1
after false
err in e 1
222
exit=0
invoke 0
stat 111
BASH_TRAPSIG = USR1
func=7

== status 0
