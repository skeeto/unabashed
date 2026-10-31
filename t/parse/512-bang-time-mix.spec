== stdout 76
f () 
{ 
    time ! a;
    time a;
    time -p ! a | b;
    a && time ! b
}

== stderr 0

== merged 76
f () 
{ 
    time ! a;
    time a;
    time -p ! a | b;
    a && time ! b
}

== status 0
