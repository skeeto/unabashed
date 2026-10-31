== stdout 75
f () 
{ 
    a && b || c;
    a || b && c;
    a && b && c;
    ! a || b
}

== stderr 0

== merged 75
f () 
{ 
    a && b || c;
    a || b && c;
    a && b && c;
    ! a || b
}

== status 0
