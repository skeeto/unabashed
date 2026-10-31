== stdout 23
f () 
{ 
    echo hi
}

== stderr 128
bash: -c: line 3: syntax error near unexpected token `;' while looking for matching `)'
bash: -c: line 3: `f() { x=$(a=(;)); }'

== merged 151
bash: -c: line 3: syntax error near unexpected token `;' while looking for matching `)'
bash: -c: line 3: `f() { x=$(a=(;)); }'
f () 
{ 
    echo hi
}

== status 0
