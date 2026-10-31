== stdout 65
f () 
{ 
    (( x  + 1 ));
    ((x+1));
    ((x+1)) && ((x+1))
}

== stderr 0

== merged 65
f () 
{ 
    (( x  + 1 ));
    ((x+1));
    ((x+1)) && ((x+1))
}

== status 0
