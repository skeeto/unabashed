== stdout 121
f () 
{ 
    (( (a) ));
    ( ( a ) );
    ( ( a ) );
    ( ( echo a );
    echo b );
    ( ( a ) );
    ( ( a ) | b )
}

== stderr 0

== merged 121
f () 
{ 
    (( (a) ));
    ( ( a ) );
    ( ( a ) );
    ( ( echo a );
    echo b );
    ( ( a ) );
    ( ( a ) | b )
}

== status 0
