== stdout 43
f () 
{ 
    echo x#y z#w;
    echo kept
}

== stderr 0

== merged 43
f () 
{ 
    echo x#y z#w;
    echo kept
}

== status 0
