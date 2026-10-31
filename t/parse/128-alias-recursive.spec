== stdout 30
f () 
{ 
    ls -F;
    a x
}

== stderr 0

== merged 30
f () 
{ 
    ls -F;
    a x
}

== status 0
