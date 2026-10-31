== stdout 60
f () 
{ 
    echo "$(echo ${x-a b})" "${ echo ${x:-c}; }"
}

== stderr 0

== merged 60
f () 
{ 
    echo "$(echo ${x-a b})" "${ echo ${x:-c}; }"
}

== status 0
