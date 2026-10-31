== stdout 30
g () 
{ 
    echo "${x:-'}"
}

== stderr 0

== merged 30
g () 
{ 
    echo "${x:-'}"
}

== status 0
