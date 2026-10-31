== stdout 100
f () 
{ 
    if a; then
        b;
    fi;
    for i in 1 2;
    do
        :;
    done;
    x=12
}

== stderr 0

== merged 100
f () 
{ 
    if a; then
        b;
    fi;
    for i in 1 2;
    do
        :;
    done;
    x=12
}

== status 0
