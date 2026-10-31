== stdout 60
f () 
{ 
    a=$(x
y; z
w);
    b=$(x; y);
    c=$(x | y)
}

== stderr 0

== merged 60
f () 
{ 
    a=$(x
y; z
w);
    b=$(x; y);
    c=$(x | y)
}

== status 0
