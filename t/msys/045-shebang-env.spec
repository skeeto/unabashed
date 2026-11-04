== stdout 103
env sh 1
env bash 2
#!/usr/bin/env cat
hello
'nosuchinterpreter': No such file or directory
status 127

== stderr 0

== merged 103
env sh 1
env bash 2
#!/usr/bin/env cat
hello
'nosuchinterpreter': No such file or directory
status 127

== status 0
