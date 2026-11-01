== stdout 60
-- a
[out]
[err]
err
-- b
[out]
-- c
[out]
[err]
-- d
[err]

== stderr 4
out

== merged 64
-- a
[out]
[err]
err
-- b
[out]
-- c
[out]
[err]
out
-- d
[err]

== status 0
