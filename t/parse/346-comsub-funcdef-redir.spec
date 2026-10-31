== stdout 147
f () 
{ 
    x=$(function g () 
{ 
    a
} > out; g);
    y=$(function h () 
{ 
    ( b ) 2>&1
});
    z=$(function k () 
{ 
    cat
} <<A
z
A
)
}

== stderr 0

== merged 147
f () 
{ 
    x=$(function g () 
{ 
    a
} > out; g);
    y=$(function h () 
{ 
    ( b ) 2>&1
});
    z=$(function k () 
{ 
    cat
} <<A
z
A
)
}

== status 0
