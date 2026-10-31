== stdout 128
f () 
{ 
    a 3> three;
    a 9< nine;
    a 10> ten;
    a 3>> app;
    a 5<> rw;
    a > one;
    a < zero;
    a 7>| clob
}

== stderr 0

== merged 128
f () 
{ 
    a 3> three;
    a 9< nine;
    a 10> ten;
    a 3>> app;
    a 5<> rw;
    a > one;
    a < zero;
    a 7>| clob
}

== status 0
