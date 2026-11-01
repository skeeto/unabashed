== stdout 128
[g1]
[g2]
[s1]
[s2]
[i1]
[f1]
[f2]
[f3]
[c1]
[w0]
[w1]
empty-ok
cond
arith
status 1
status 1
status 1
between
status 1
status 1

== stderr 274
bash: line 20: /tmp/nodir/f: No such file or directory
bash: line 24: /tmp/nodir/g: No such file or directory
bash: line 28: /tmp/nosuch: No such file or directory
bash: line 30: /tmp/nodir/h: No such file or directory
bash: line 38: /tmp/nodir/i: No such file or directory

== merged 402
[g1]
[g2]
[s1]
[s2]
[i1]
[f1]
[f2]
[f3]
[c1]
[w0]
[w1]
empty-ok
cond
arith
bash: line 20: /tmp/nodir/f: No such file or directory
status 1
bash: line 24: /tmp/nodir/g: No such file or directory
status 1
bash: line 28: /tmp/nosuch: No such file or directory
status 1
between
bash: line 30: /tmp/nodir/h: No such file or directory
status 1
bash: line 38: /tmp/nodir/i: No such file or directory
status 1

== status 0
