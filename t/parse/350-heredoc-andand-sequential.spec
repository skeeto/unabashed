== stdout 62
f () 
{ 
    cat <<A && 
a
A
 cat <<B && 
b
B
 cat <<C
c
C

}

== stderr 0

== merged 62
f () 
{ 
    cat <<A && 
a
A
 cat <<B && 
b
B
 cat <<C
c
C

}

== status 0
