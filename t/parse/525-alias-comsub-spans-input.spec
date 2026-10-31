== stdout 47
f () 
{ 
    echo $(echo hi);
    echo after
}

== stderr 0

== merged 47
f () 
{ 
    echo $(echo hi);
    echo after
}

== status 0
