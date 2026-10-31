== stdout 139
f () 
{ 
    { 
        { 
            cat <<A
a
A

        } | b
    } && c;
    x;
    { 
        y;
        cat <<B
b
B

    };
    z
}

== stderr 0

== merged 139
f () 
{ 
    { 
        { 
            cat <<A
a
A

        } | b
    } && c;
    x;
    { 
        y;
        cat <<B
b
B

    };
    z
}

== status 0
