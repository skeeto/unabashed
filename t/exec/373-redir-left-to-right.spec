== stdout 72
-- a
-- b
[payload]
status 1
c status 1 []
d status 1
[v]
[via3]
[via4]

== stderr 101
bash: line 6: /tmp/nosuch: No such file or directory
bash: line 9: /tmp/d: No such file or directory

== merged 173
-- a
-- b
[payload]
bash: line 6: /tmp/nosuch: No such file or directory
status 1
c status 1 []
bash: line 9: /tmp/d: No such file or directory
d status 1
[v]
[via3]
[via4]

== status 0
