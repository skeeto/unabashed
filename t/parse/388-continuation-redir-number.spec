== stdout 42
f () 
{ 
    a b 2> x {fd}> y 3<&4 1>&2
}

== stderr 0

== merged 42
f () 
{ 
    a b 2> x {fd}> y 3<&4 1>&2
}

== status 0
