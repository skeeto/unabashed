== stdout 59
f () 
{ 
    echo 'AJ2' 'A' 'AB' '' '\xg' '\x' 'a' 'c'
}

== stderr 0

== merged 59
f () 
{ 
    echo 'AJ2' 'A' 'AB' '' '\xg' '\x' 'a' 'c'
}

== status 0
