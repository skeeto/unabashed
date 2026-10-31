== stdout 204
f () 
{ 
    function g () 
    { 
        cat
    } <<A
a
A

    echo x
    function h () 
    { 
        cat
    } <<B && 
b
B
 echo y
    function k () 
    { 
        cat <<C
c
C

    };
    echo z
}

== stderr 0

== merged 204
f () 
{ 
    function g () 
    { 
        cat
    } <<A
a
A

    echo x
    function h () 
    { 
        cat
    } <<B && 
b
B
 echo y
    function k () 
    { 
        cat <<C
c
C

    };
    echo z
}

== status 0
