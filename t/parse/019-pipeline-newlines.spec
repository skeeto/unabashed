== stdout 42
f () 
{ 
    a | b | c;
    d && e || g
}

== stderr 0

== merged 42
f () 
{ 
    a | b | c;
    d && e || g
}

== status 0
