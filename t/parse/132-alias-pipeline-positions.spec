== stdout 192
f () 
{ 
    echo X | echo X && echo X || echo X;
    echo X & echo X;
    ! echo X;
    time echo X;
    ( echo X );
    { 
        echo X
    };
    if echo X; then
        echo X;
    fi
}

== stderr 0

== merged 192
f () 
{ 
    echo X | echo X && echo X || echo X;
    echo X & echo X;
    ! echo X;
    time echo X;
    ( echo X );
    { 
        echo X
    };
    if echo X; then
        echo X;
    fi
}

== status 0
