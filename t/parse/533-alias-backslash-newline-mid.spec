== stdout 41
f () 
{ 
    echo ab c x;
    echo d y
}

== stderr 83
bash: -c: line 10: syntax error near unexpected token `fi'
bash: -c: line 10: `fi'

== merged 124
f () 
{ 
    echo ab c x;
    echo d y
}
bash: -c: line 10: syntax error near unexpected token `fi'
bash: -c: line 10: `fi'

== status 2
