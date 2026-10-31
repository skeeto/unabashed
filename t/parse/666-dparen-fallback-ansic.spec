== stdout 57
f () 
{ 
    ( ( echo 'a';
    echo "${x:-}" ) )
}

== stderr 0

== merged 57
f () 
{ 
    ( ( echo 'a';
    echo "${x:-}" ) )
}

== status 0
