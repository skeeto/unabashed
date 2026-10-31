== stdout 64
f () 
{ 
    echo x;
    'x';
    \x;
    "x";
    a=1 echo X
}

== stderr 0

== merged 64
f () 
{ 
    echo x;
    'x';
    \x;
    "x";
    a=1 echo X
}

== status 0
