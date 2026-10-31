== stdout 120
f () 
{ 
    cat $(cat <<B
b
B
) more <<A
a
A

    cat $(cat <<C
c
C
) <<D
d
D

    echo $(cat <<E
e
E
)
    echo end
}

== stderr 0

== merged 120
f () 
{ 
    cat $(cat <<B
b
B
) more <<A
a
A

    cat $(cat <<C
c
C
) <<D
d
D

    echo $(cat <<E
e
E
)
    echo end
}

== status 0
