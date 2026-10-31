== stdout 50
f () 
{ 
    echo brace x
}
g () 
{ 
    echo {
}

== stderr 0

== merged 50
f () 
{ 
    echo brace x
}
g () 
{ 
    echo {
}

== status 0
