== stdout 59
status 1
status 1
status 1
status 1
status 1
008-cd-errors

== stderr 269
test.sh: line 3: cd: /no/such/dir: No such file or directory
test.sh: line 4: cd: C:/no/such/dir: No such file or directory
test.sh: line 5: cd: C:\no\such\dir: No such file or directory
test.sh: line 6: cd: test.sh: Not a directory
test.sh: line 7: cd: null directory

== merged 328
test.sh: line 3: cd: /no/such/dir: No such file or directory
status 1
test.sh: line 4: cd: C:/no/such/dir: No such file or directory
status 1
test.sh: line 5: cd: C:\no\such\dir: No such file or directory
status 1
test.sh: line 6: cd: test.sh: Not a directory
status 1
test.sh: line 7: cd: null directory
status 1
008-cd-errors

== status 0
