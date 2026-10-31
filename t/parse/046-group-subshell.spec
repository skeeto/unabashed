== stdout 142
f () 
{ 
    { 
        a;
        b
    };
    ( a;
    b );
    { 
        a
    } && ( b );
    { 
        a;
        b
    };
    ( a )
}

== stderr 0

== merged 142
f () 
{ 
    { 
        a;
        b
    };
    ( a;
    b );
    { 
        a
    } && ( b );
    { 
        a;
        b
    };
    ( a )
}

== status 0
