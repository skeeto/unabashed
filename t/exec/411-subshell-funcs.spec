== stdout 133
inner-f
outer-f
bash: line 5: f: command not found
st=127
outer-f
g
bash: line 8: g: command not found
g status: 127
declare -F g: 1

== stderr 0

== merged 133
inner-f
outer-f
bash: line 5: f: command not found
st=127
outer-f
g
bash: line 8: g: command not found
g status: 127
declare -F g: 1

== status 0
