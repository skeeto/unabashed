== stdout 58
f () 
{ 
    echo $(echo a) ${x} $((1+2)) a$b $$ "${y}"
}

== stderr 0

== merged 58
f () 
{ 
    echo $(echo a) ${x} $((1+2)) a$b $$ "${y}"
}

== status 0
