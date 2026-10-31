== stdout 136
f () 
{ 
    a b > out;
    a b > out;
    a 2>&1 > out;
    < in;
    > out;
    x=1 > out;
    x=1 a > out;
    a < in > out 2> err
}

== stderr 0

== merged 136
f () 
{ 
    a b > out;
    a b > out;
    a 2>&1 > out;
    < in;
    > out;
    x=1 > out;
    x=1 a > out;
    a < in > out 2> err
}

== status 0
