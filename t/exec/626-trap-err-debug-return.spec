== stdout 385
err 1 at 4
in f
err 1 at 11
st 1
err 1 at 13
err 1 at 14
err 1 at 16
err 3 at 18
err 1 at 19
err 1 at 20
err 1 at 21
err 1 at 23
err 1 at 24
err 1 at 5
in f
err 1 at 27
err 1 at 27
---
dbg [h] 0
inh
dbg [x=1] 0
dbg [(( x ))] 0
dbg [trap - DEBUG] 0
---
ink
after 4
ink
ret k
---
in source
ret 0
in source
ret 0
ret 0
D for i in a b
D :
D for i in a b
D :
D trap - DEBUG
e 5
e 1
exitt 1

== stderr 0

== merged 385
err 1 at 4
in f
err 1 at 11
st 1
err 1 at 13
err 1 at 14
err 1 at 16
err 3 at 18
err 1 at 19
err 1 at 20
err 1 at 21
err 1 at 23
err 1 at 24
err 1 at 5
in f
err 1 at 27
err 1 at 27
---
dbg [h] 0
inh
dbg [x=1] 0
dbg [(( x ))] 0
dbg [trap - DEBUG] 0
---
ink
after 4
ink
ret k
---
in source
ret 0
in source
ret 0
ret 0
D for i in a b
D :
D for i in a b
D :
D trap - DEBUG
e 5
e 1
exitt 1

== status 1
