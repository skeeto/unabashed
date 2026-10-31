== stdout 108
f () 
{ 
    time -p a b;
    time -p a;
    time -p ;
    time -p ! a;
    time -p -p;
    a | time -p b
}

== stderr 0

== merged 108
f () 
{ 
    time -p a b;
    time -p a;
    time -p ;
    time -p ! a;
    time -p -p;
    a | time -p b
}

== status 0
