== stdout 170
f () 
{ 
    ( a ) > out
}
g () 
{ 
    a
} 2>&1 > o
h () 
{ 
    [[ -n x ]] 2> /dev/null
}
k () 
{ 
    a
} {v}>&- 3<&4-
m () 
{ 
    if a; then
        b;
    fi > o
}

== stderr 0

== merged 170
f () 
{ 
    ( a ) > out
}
g () 
{ 
    a
} 2>&1 > o
h () 
{ 
    [[ -n x ]] 2> /dev/null
}
k () 
{ 
    a
} {v}>&- 3<&4-
m () 
{ 
    if a; then
        b;
    fi > o
}

== status 0
