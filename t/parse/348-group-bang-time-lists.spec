== stdout 151
f () 
{ 
    a && ! { 
        b
    };
    time { 
        a
    } | b;
    ! ( a ) || { 
        b
    } & { 
        a
    } & { 
        b
    }
}

== stderr 0

== merged 151
f () 
{ 
    a && ! { 
        b
    };
    time { 
        a
    } | b;
    ! ( a ) || { 
        b
    } & { 
        a
    } & { 
        b
    }
}

== status 0
