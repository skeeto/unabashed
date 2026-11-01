== stdout 50
status 1
status 1
status 1
status 1
still running

== stderr 185
bash: line 2: /tmp/nosuch: No such file or directory
bash: line 4: /tmp/nodir/x: No such file or directory
bash: line 6: 8: Bad file descriptor
bash: line 8: $empty: ambiguous redirect

== merged 235
bash: line 2: /tmp/nosuch: No such file or directory
status 1
bash: line 4: /tmp/nodir/x: No such file or directory
status 1
bash: line 6: 8: Bad file descriptor
status 1
bash: line 8: $empty: ambiguous redirect
status 1
still running

== status 0
