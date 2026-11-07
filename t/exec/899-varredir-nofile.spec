== stdout 44
v=10
status 1 10
status 0
unset
status 0 10

== stderr 202
bash: redirection error: cannot duplicate fd: Invalid argument
bash: line 4: /dev/null: Invalid argument
bash: redirection error: cannot duplicate fd: Invalid argument
bash: line 5: 2: Invalid argument

== merged 246
v=10
bash: redirection error: cannot duplicate fd: Invalid argument
bash: line 4: /dev/null: Invalid argument
status 1 10
status 0
bash: redirection error: cannot duplicate fd: Invalid argument
bash: line 5: 2: Invalid argument
unset
status 0 10

== status 0
