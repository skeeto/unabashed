== stdout 80
sh 1
env-bash 2
one  two ./c 3
one  two ./c ./d 4
plain 5
status 126
status 127

== stderr 124
bash: ./f: /no/such/interpreter: bad interpreter: No such file or directory
/usr/bin/env: 'fake': No such file or directory

== merged 204
sh 1
env-bash 2
one  two ./c 3
one  two ./c ./d 4
plain 5
bash: ./f: /no/such/interpreter: bad interpreter: No such file or directory
status 126
/usr/bin/env: 'fake': No such file or directory
status 127

== status 0
