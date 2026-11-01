== stdout 50
one
three
trace file:
+ echo one
+ : two
+ set +x

== stderr 94
bash: line 3: BASH_XTRACEFD: 7: invalid value for trace file descriptor
+ echo three
+ set +x

== merged 144
bash: line 3: BASH_XTRACEFD: 7: invalid value for trace file descriptor
one
+ echo three
three
+ set +x
trace file:
+ echo one
+ : two
+ set +x

== status 0
