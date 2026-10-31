== stdout 59
f () 
{ 
    [[ -n a && -n b || -n c ]];
    [[ -n $x ]]
}

== stderr 0

== merged 59
f () 
{ 
    [[ -n a && -n b || -n c ]];
    [[ -n $x ]]
}

== status 0
