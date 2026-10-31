== stdout 96
f () 
{ 
    ( ( echo b ) )
}
g () 
{ 
    ( ( c );
    ( d ) )
}
h () 
{ 
    ( ( echo a ) )
}

== stderr 0

== merged 96
f () 
{ 
    ( ( echo b ) )
}
g () 
{ 
    ( ( c );
    ( d ) )
}
h () 
{ 
    ( ( echo a ) )
}

== status 0
