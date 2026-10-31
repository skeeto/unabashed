== stdout 62
f () 
{ 
    ((1+2));
    ( ( a ) );
    echo $((3)) $((4))
}

== stderr 0

== merged 62
f () 
{ 
    ((1+2));
    ( ( a ) );
    echo $((3)) $((4))
}

== status 0
