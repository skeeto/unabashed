== stdout 41
f () 
{ 
    echo a\ b;
    echo c\\ d
}

== stderr 0

== merged 41
f () 
{ 
    echo a\ b;
    echo c\\ d
}

== status 0
