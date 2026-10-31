== stdout 60
f () 
{ 
    echo '' '' 'abc' '' "${x:-}" ${x#''}
}

== stderr 0

== merged 60
f () 
{ 
    echo '' '' 'abc' '' "${x:-}" ${x#''}
}

== status 0
