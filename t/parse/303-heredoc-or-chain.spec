== stdout 53
f () 
{ 
    a <<A || 
1
A
 b <<B && 
2
B
 c
    d
}

== stderr 0

== merged 53
f () 
{ 
    a <<A || 
1
A
 b <<B && 
2
B
 c
    d
}

== status 0
