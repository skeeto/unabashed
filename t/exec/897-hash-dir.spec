== stdout 113
status 1
status 0
status 1
status 0
builtin hash -p /opt/fakes/bin/cat kitty
status 0
status 0
status 1
status 0

== stderr 150
bash: line 3: hash: /: Is a directory
bash: line 5: hash: /: Is a directory
bash: line 5: hash: /: Is a directory
bash: line 10: hash: ./x: not found

== merged 263
bash: line 3: hash: /: Is a directory
status 1
status 0
bash: line 5: hash: /: Is a directory
bash: line 5: hash: /: Is a directory
status 1
status 0
builtin hash -p /opt/fakes/bin/cat kitty
status 0
status 0
bash: line 10: hash: ./x: not found
status 1
status 0

== status 0
