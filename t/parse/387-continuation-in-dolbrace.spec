== stdout 63
f () 
{ 
    echo ${x:-ab} "${y:-c}" ${z///q} ${#w} $((1+2))
}

== stderr 0

== merged 63
f () 
{ 
    echo ${x:-ab} "${y:-c}" ${z///q} ${#w} $((1+2))
}

== status 0
