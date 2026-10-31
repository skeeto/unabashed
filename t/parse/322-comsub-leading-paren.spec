== stdout 139
f () 
{ 
    a=$( ( x ) | y);
    b=$( ( x ); y);
    c=$( ( x ) && y);
    d=<( ( x ));
    e=$( ((x)));
    echo >( ( y )) "$( ( q ))"
}

== stderr 0

== merged 139
f () 
{ 
    a=$( ( x ) | y);
    b=$( ( x ); y);
    c=$( ( x ) && y);
    d=<( ( x ));
    e=$( ((x)));
    echo >( ( y )) "$( ( q ))"
}

== status 0
