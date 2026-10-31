== stdout 77
f () 
{ 
    a &> out;
    a &>> out;
    a >&out;
    a >&out;
    a 1>&1
}

== stderr 0

== merged 77
f () 
{ 
    a &> out;
    a &>> out;
    a >&out;
    a >&out;
    a 1>&1
}

== status 0
