== stdout 80
f () 
{ 
    ( ( a ) );
    ((a));
    ( ( a ) || b );
    ( ( a ) && ( b ) )
}

== stderr 0

== merged 80
f () 
{ 
    ( ( a ) );
    ((a));
    ( ( a ) || b );
    ( ( a ) && ( b ) )
}

== status 0
