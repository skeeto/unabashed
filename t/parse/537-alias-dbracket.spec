== stdout 60
f () 
{ 
    [[ -n x ]];
    [[ -n x ]];
    [[ x == x ]]
}

== stderr 0

== merged 60
f () 
{ 
    [[ -n x ]];
    [[ -n x ]];
    [[ x == x ]]
}

== status 0
