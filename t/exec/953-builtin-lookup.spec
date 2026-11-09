== stdout 162
empty first word
two empty words
disabled meanwhile: 127
enabled meanwhile: 0
f: arg
fn: shadowed
plain
declare -x x="1"
y=2
z=3
via command
via builtin
status 1

== stderr 69
bash: line 7: :: command not found
bash: line 26: fg: no job control

== merged 231
empty first word
two empty words
bash: line 7: :: command not found
disabled meanwhile: 127
enabled meanwhile: 0
f: arg
fn: shadowed
plain
declare -x x="1"
y=2
z=3
via command
via builtin
bash: line 26: fg: no job control
status 1

== status 0
