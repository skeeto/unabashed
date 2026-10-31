== stdout 36
g () 
{ 
    echo $(a <<F
b
F
 c)
}

== stderr 0

== merged 36
g () 
{ 
    echo $(a <<F
b
F
 c)
}

== status 0
