== stdout 67
f () 
{ 
    time echo X;
    time -p echo X;
    time -p echo X
}

== stderr 0

== merged 67
f () 
{ 
    time echo X;
    time -p echo X;
    time -p echo X
}

== status 0
