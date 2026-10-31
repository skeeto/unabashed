== stdout 84
f () 
{ 
    a=$(x & y &);
    b=$(x & y);
    c=$(x && y & z);
    d=$(x; y & z)
}

== stderr 0

== merged 84
f () 
{ 
    a=$(x & y &);
    b=$(x & y);
    c=$(x && y & z);
    d=$(x; y & z)
}

== status 0
