== stdout 79
f () 
{ 
    cat
} <<A > out <<B
a
A
b
B

g () 
{ 
    cat <<C
c
C

} <<D
d
D


== stderr 0

== merged 79
f () 
{ 
    cat
} <<A > out <<B
a
A
b
B

g () 
{ 
    cat <<C
c
C

} <<D
d
D


== status 0
