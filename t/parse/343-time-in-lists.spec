== stdout 124
f () 
{ 
    a && time b || ! c;
    time a & time b & a | time b;
    time a && time -p b;
    x;
    time ;
    time  &
}

== stderr 0

== merged 124
f () 
{ 
    a && time b || ! c;
    time a & time b & a | time b;
    time a && time -p b;
    x;
    time ;
    time  &
}

== status 0
