== stdout 25
start
exit 1
no cwd file

== stderr 61
/usr/bin/bash: line 1: cd: nosuch: No such file or directory

== merged 86
start
/usr/bin/bash: line 1: cd: nosuch: No such file or directory
exit 1
no cwd file

== status 1
