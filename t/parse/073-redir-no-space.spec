== stdout 107
f () 
{ 
    a > out;
    a < in 2>&1 > out;
    a >> app < in;
    a2 > x;
    echo > x;
    echo 12> x
}

== stderr 0

== merged 107
f () 
{ 
    a > out;
    a < in 2>&1 > out;
    a >> app < in;
    a2 > x;
    echo > x;
    echo 12> x
}

== status 0
