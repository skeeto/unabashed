== stdout 134
f () 
{ 
    ( cat <<A
a
A
 ) | b;
    { 
        cat <<B
b
B

    } && c;
    ( { 
        cat <<C
c
C

    } ) || d;
    echo end
}

== stderr 0

== merged 134
f () 
{ 
    ( cat <<A
a
A
 ) | b;
    { 
        cat <<B
b
B

    } && c;
    ( { 
        cat <<C
c
C

    } ) || d;
    echo end
}

== status 0
