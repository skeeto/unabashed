== stdout 117
dbg 1
dbg 2
dbg 2
dbg 3
dbg 3
dbg 1
dbg 1
dbg 2
dbg 2
dbg 2
dbg 3
dbg 3
fdbg 1
fdbg 2
fdbg 2
+ for i in 1 2
+ set -x

== stderr 9
+ set +x

== merged 126
dbg 1
dbg 2
dbg 2
dbg 3
dbg 3
dbg 1
dbg 1
dbg 2
dbg 2
dbg 2
dbg 3
dbg 3
fdbg 1
fdbg 2
fdbg 2
+ for i in 1 2
+ set -x
+ set +x

== status 0
