== stdout 146
f () 
{ 
    time a;
    time a | b;
    time -p a;
    time ! a;
    time ! a;
    time { 
        a;
        b
    };
    time ;
    time -p 
}

== stderr 0

== merged 146
f () 
{ 
    time a;
    time a | b;
    time -p a;
    time ! a;
    time ! a;
    time { 
        a;
        b
    };
    time ;
    time -p 
}

== status 0
