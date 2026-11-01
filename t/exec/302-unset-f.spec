== stdout 83
f runs
status 127
unset missing status 0
declare -F status 1
g var: var
status 127

== stderr 71
bash: line 4: f: command not found
bash: line 14: g: command not found

== merged 154
f runs
bash: line 4: f: command not found
status 127
unset missing status 0
declare -F status 1
g var: var
bash: line 14: g: command not found
status 127

== status 0
