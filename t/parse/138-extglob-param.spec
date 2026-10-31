== stdout 67
f () 
{ 
    echo ${x##+([[:space:]])} ${x%%*( )} ${x//@(a|b)/c}
}

== stderr 0

== merged 67
f () 
{ 
    echo ${x##+([[:space:]])} ${x%%*( )} ${x//@(a|b)/c}
}

== status 0
