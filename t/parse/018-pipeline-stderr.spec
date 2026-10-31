== stdout 67
f () 
{ 
    a 2>&1 | b;
    a 2>&1 | b;
    a 2>&1 | b 2>&1 | c
}

== stderr 0

== merged 67
f () 
{ 
    a 2>&1 | b;
    a 2>&1 | b;
    a 2>&1 | b 2>&1 | c
}

== status 0
