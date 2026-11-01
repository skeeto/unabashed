== stdout 76
fd 3 closed in parent
f3: via3
captured
fd 4 closed
f5: from-sub
f5: parent

== stderr 74
bash: line 3: 3: Bad file descriptor
bash: line 7: 4: Bad file descriptor

== merged 150
bash: line 3: 3: Bad file descriptor
fd 3 closed in parent
f3: via3
captured
bash: line 7: 4: Bad file descriptor
fd 4 closed
f5: from-sub
f5: parent

== status 0
