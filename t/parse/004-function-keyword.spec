== stdout 92
f () 
{ 
    echo f
}
g () 
{ 
    echo g
}
h () 
{ 
    ( echo h )
}
i () 
{ 
    echo i
}

== stderr 0

== merged 92
f () 
{ 
    echo f
}
g () 
{ 
    echo g
}
h () 
{ 
    ( echo h )
}
i () 
{ 
    echo i
}

== status 0
