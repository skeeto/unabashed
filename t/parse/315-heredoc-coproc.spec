== stdout 129
f () 
{ 
    coproc cat <<A
a
A

    coproc C { 
        cat
    } <<B
b
B

    coproc COPROC ( cat <<D
d
D
 );
    echo after
}

== stderr 0

== merged 129
f () 
{ 
    coproc cat <<A
a
A

    coproc C { 
        cat
    } <<B
b
B

    coproc COPROC ( cat <<D
d
D
 );
    echo after
}

== status 0
