== stdout 30
one
f () 
{ 
    echo f
}
two

== stderr 85
bash: -c: line 5: syntax error near unexpected token `done'
bash: -c: line 5: `done'

== merged 115
one
f () 
{ 
    echo f
}
two
bash: -c: line 5: syntax error near unexpected token `done'
bash: -c: line 5: `done'

== status 2
