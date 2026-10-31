== stdout 73
f () 
{ 
    echo one;
    two
}
g () 
{ 
    echo one;
    echo three
}

== stderr 0

== merged 73
f () 
{ 
    echo one;
    two
}
g () 
{ 
    echo one;
    echo three
}

== status 0
