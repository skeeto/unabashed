== stdout 78
f () 
{ 
    echo a ]] b [[;
    x=[[ y=]];
    echo [[x ]]y;
    a [[ b ]]
}

== stderr 83
bash: -c: line 8: syntax error near unexpected token `]]'
bash: -c: line 8: `]] a'

== merged 161
f () 
{ 
    echo a ]] b [[;
    x=[[ y=]];
    echo [[x ]]y;
    a [[ b ]]
}
bash: -c: line 8: syntax error near unexpected token `]]'
bash: -c: line 8: `]] a'

== status 2
