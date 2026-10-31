== stdout 32
f () 
{ 
    ( ( ${x//)/} ) )
}

== stderr 0

== merged 32
f () 
{ 
    ( ( ${x//)/} ) )
}

== status 0
