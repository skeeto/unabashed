== stdout 97
f () 
{ 
    [[ ( -n k=v ) ]]
}
g () 
{ 
    [[ -n a && -n x=1 ]]
}
h () 
{ 
    [[ ! -n if ]]
}

== stderr 0

== merged 97
f () 
{ 
    [[ ( -n k=v ) ]]
}
g () 
{ 
    [[ -n a && -n x=1 ]]
}
h () 
{ 
    [[ ! -n if ]]
}

== status 0
