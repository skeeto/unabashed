== stdout 267
f () 
{ 
    ( { 
        ( a;
        b )
    } );
    { 
        ( a );
        { 
            b
        }
    };
    ( a;
    { 
        b;
        c
    };
    d );
    ( a | { 
        b;
        c
    } );
    { 
        a;
        ( b;
        c )
    } | d
}

== stderr 0

== merged 267
f () 
{ 
    ( { 
        ( a;
        b )
    } );
    { 
        ( a );
        { 
            b
        }
    };
    ( a;
    { 
        b;
        c
    };
    d );
    ( a | { 
        b;
        c
    } );
    { 
        a;
        ( b;
        c )
    } | d
}

== status 0
