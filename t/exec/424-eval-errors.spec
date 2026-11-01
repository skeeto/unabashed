== stdout 205
bash: line 2: unset_cmd_xyz: command not found
st=127
dashdash
bash: line 5: eval: -x: invalid option
eval: usage: eval [arg ...]
opt st=2
bash: line 7: ro: readonly variable
ro st=1
bash: line 9: x: no x

== stderr 0

== merged 205
bash: line 2: unset_cmd_xyz: command not found
st=127
dashdash
bash: line 5: eval: -x: invalid option
eval: usage: eval [arg ...]
opt st=2
bash: line 7: ro: readonly variable
ro st=1
bash: line 9: x: no x

== status 127
