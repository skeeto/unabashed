== stdout 176
f () 
{ 
    while a; do
        b;
    done;
    until a; do
        b;
    done;
    while a;
    b; do
        c;
        d;
    done;
    while a; do
        b;
    done
}

== stderr 0

== merged 176
f () 
{ 
    while a; do
        b;
    done;
    until a; do
        b;
    done;
    while a;
    b; do
        c;
        d;
    done;
    while a; do
        b;
    done
}

== status 0
