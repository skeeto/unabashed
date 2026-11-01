== stdout 161
got line 1
got line 2
got line 3
count 3
g: if out
g: case out
g: grp
g: arith out
bad redirect: 1
missing input: 1
if bad redirect: 1
multi-line: 1
if multi: 1

== stderr 282
bash: line 13: /nonexistent/x: No such file or directory
bash: line 15: /tmp/missing: No such file or directory
bash: line 17: /nonexistent/y: No such file or directory
bash: line 22: /tmp/missing2: No such file or directory
bash: line 26: /nonexistent/z: No such file or directory

== merged 443
got line 1
got line 2
got line 3
count 3
g: if out
g: case out
g: grp
g: arith out
bash: line 13: /nonexistent/x: No such file or directory
bad redirect: 1
bash: line 15: /tmp/missing: No such file or directory
missing input: 1
bash: line 17: /nonexistent/y: No such file or directory
if bad redirect: 1
bash: line 22: /tmp/missing2: No such file or directory
multi-line: 1
bash: line 26: /nonexistent/z: No such file or directory
if multi: 1

== status 0
