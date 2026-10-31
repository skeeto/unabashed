== stdout 96
f () 
{ 
    echo f
}
g () 
{ 
    ( echo g )
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

== merged 96
f () 
{ 
    echo f
}
g () 
{ 
    ( echo g )
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
