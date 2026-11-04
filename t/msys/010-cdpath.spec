== stdout 69
@WORK@/010-cdpath/a/b
/a/b
@WORK@/010-cdpath/c/d
/c/d
status 1 
/a/b

== stderr 51
test.sh: line 10: cd: b: No such file or directory

== merged 120
@WORK@/010-cdpath/a/b
/a/b
@WORK@/010-cdpath/c/d
/c/d
test.sh: line 10: cd: b: No such file or directory
status 1 
/a/b

== status 0
