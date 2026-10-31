== stdout 70
f () 
{ 
    cat <<A && 
a
A
 echo ok || cat <<B
b
B

    echo last
}

== stderr 0

== merged 70
f () 
{ 
    cat <<A && 
a
A
 echo ok || cat <<B
b
B

    echo last
}

== status 0
