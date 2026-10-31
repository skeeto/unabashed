== stdout 51
f () 
{ 
    a;
    b & c && d | e || g;
    h &
}

== stderr 0

== merged 51
f () 
{ 
    a;
    b & c && d | e || g;
    h &
}

== status 0
