== stdout 71
f () 
{ 
    a=$({ x
y; });
    b=$( ( x
y ));
    c=$({ x; } > o
y)
}

== stderr 0

== merged 71
f () 
{ 
    a=$({ x
y; });
    b=$( ( x
y ));
    c=$({ x; } > o
y)
}

== status 0
