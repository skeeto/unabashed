== stdout 89
f () 
{ 
    echo > x 2> y 7< z;
    echo a 1>&2 0<&0;
    echo 3<<E
x
E

    a {x}>&4
}

== stderr 0

== merged 89
f () 
{ 
    echo > x 2> y 7< z;
    echo a 1>&2 0<&0;
    echo 3<<E
x
E

    a {x}>&4
}

== status 0
