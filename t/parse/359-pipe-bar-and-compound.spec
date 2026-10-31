== stdout 159
f () 
{ 
    if a; then
        b;
    fi 2>&1 | c;
    ( a ) 2> x 2>&1 | b;
    while a; do
        b;
    done 2>&1 | c 2>&1 | d;
    a <<A 2>&1 |
x
A
  b
}

== stderr 0

== merged 159
f () 
{ 
    if a; then
        b;
    fi 2>&1 | c;
    ( a ) 2> x 2>&1 | b;
    while a; do
        b;
    done 2>&1 | c 2>&1 | d;
    a <<A 2>&1 |
x
A
  b
}

== status 0
