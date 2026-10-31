== stdout 78
f () 
{ 
    a &
}
g () 
{ 
    a & b &
}
h () 
{ 
    { 
        a
    } &
}

== stderr 0

== merged 78
f () 
{ 
    a &
}
g () 
{ 
    a & b &
}
h () 
{ 
    { 
        a
    } &
}

== status 0
