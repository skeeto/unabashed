== stdout 107
after plain unset: [unset]
func both
status 127
onlyf
[unset]
status 0
status 0
status 1
status 0
status 1

== stderr 166
bash: line 7: both: command not found
bash: line 19: unset: `a b': not a valid identifier
bash: line 23: unset: cannot simultaneously unset a function and a variable

== merged 273
after plain unset: [unset]
func both
bash: line 7: both: command not found
status 127
onlyf
[unset]
status 0
status 0
bash: line 19: unset: `a b': not a valid identifier
status 1
status 0
bash: line 23: unset: cannot simultaneously unset a function and a variable
status 1

== status 0
