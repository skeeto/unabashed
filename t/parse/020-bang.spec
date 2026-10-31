== stdout 110
f () 
{ 
    ! a;
    ! a | b;
    a;
    ! { 
        a
    };
    ! ( a );
    ! [[ -n x ]];
    a && ! b
}

== stderr 0

== merged 110
f () 
{ 
    ! a;
    ! a | b;
    a;
    ! { 
        a
    };
    ! ( a );
    ! [[ -n x ]];
    a && ! b
}

== status 0
