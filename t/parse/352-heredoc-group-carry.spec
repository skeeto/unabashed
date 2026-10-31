== stdout 111
f () 
{ 
    { 
        cat <<A
a
A

    };
    echo one;
    echo two;
    ( cat <<B
b
B
 );
    echo three
}

== stderr 0

== merged 111
f () 
{ 
    { 
        cat <<A
a
A

    };
    echo one;
    echo two;
    ( cat <<B
b
B
 );
    echo three
}

== status 0
