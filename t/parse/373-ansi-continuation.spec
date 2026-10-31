== stdout 38
f () 
{ 
    echo 'a\
b' 'c	d' "ef"
}

== stderr 0

== merged 38
f () 
{ 
    echo 'a\
b' 'c	d' "ef"
}

== status 0
