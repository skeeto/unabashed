== stdout 62
f () 
{ 
    ! echo X;
    echo X;
    ! echo X;
    echo X
}

== stderr 0

== merged 62
f () 
{ 
    ! echo X;
    echo X;
    ! echo X;
    echo X
}

== status 0
