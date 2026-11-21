== stdout 170
redefined
x=[eval: line 10: syntax error near unexpected token `('
bash: eval: line 10: `echo hello ( x y z']
bee
st=3
r again
f 4
r third
sub 5
x again
comsub 6
y again

== stderr 100
bash: eval: line 6: syntax error near unexpected token `('
bash: eval: line 6: `echo hello ( x y z'

== merged 270
bash: eval: line 6: syntax error near unexpected token `('
bash: eval: line 6: `echo hello ( x y z'
redefined
x=[eval: line 10: syntax error near unexpected token `('
bash: eval: line 10: `echo hello ( x y z']
bee
st=3
r again
f 4
r third
sub 5
x again
comsub 6
y again

== status 0
