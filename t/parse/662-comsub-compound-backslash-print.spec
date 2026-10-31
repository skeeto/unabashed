== stdout 42
f () 
{ 
    echo $(y=(a\b)) <(z=(\$x))
}

== stderr 0

== merged 42
f () 
{ 
    echo $(y=(a\b)) <(z=(\$x))
}

== status 0
