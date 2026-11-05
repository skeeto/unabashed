== stdout 207
logout 0 1
trap 5
status 5
end of string
after subshell 2
logout 0 f
status 3
status 6
logout2
status 9
logout3
status 0
logout3
status 8
status 1
trap from logout
status 1
logout4
status 2
logout4
status 3

== stderr 50
bash: line 1: logout: not login shell: use `exit'

== merged 257
logout 0 1
trap 5
status 5
end of string
after subshell 2
logout 0 f
status 3
status 6
logout2
status 9
logout3
status 0
logout3
status 8
bash: line 1: logout: not login shell: use `exit'
status 1
trap from logout
status 1
logout4
status 2
logout4
status 3

== status 0
