== stdout 79
before
status 1
status 1
status 1
status 1
status 1 x=
status 1
status 1
after

== stderr 362
bash: line 3: /tmp/nosuch: No such file or directory
bash: line 5: /tmp/nodir/file: No such file or directory
bash: line 7: /tmp: Is a directory
bash: line 9: /tmp/nodir/x: No such file or directory
bash: line 11: /tmp/nosuch: No such file or directory
bash: line 14: /tmp/nosuch: No such file or directory
bash: line 16: /tmp/nodir/e: No such file or directory

== merged 441
before
bash: line 3: /tmp/nosuch: No such file or directory
status 1
bash: line 5: /tmp/nodir/file: No such file or directory
status 1
bash: line 7: /tmp: Is a directory
status 1
bash: line 9: /tmp/nodir/x: No such file or directory
status 1
bash: line 11: /tmp/nosuch: No such file or directory
status 1 x=
bash: line 14: /tmp/nosuch: No such file or directory
status 1
bash: line 16: /tmp/nodir/e: No such file or directory
status 1
after

== status 0
