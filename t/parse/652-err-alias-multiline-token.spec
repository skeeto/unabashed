== stdout 6
a
x
y

== stderr 95
bash: -c: line 6: syntax error near unexpected token `fi'
bash: -c: line 6: `echo x
echo y
f '

== merged 101
a
x
y
bash: -c: line 6: syntax error near unexpected token `fi'
bash: -c: line 6: `echo x
echo y
f '

== status 2
