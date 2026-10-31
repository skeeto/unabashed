== stdout 243
f () 
{ 
    for i in "$@";
    do
        :;
    done;
    for i in "$@";
    do
        a;
    done;
    select x in "$@";
    do
        b;
    done;
    for i in ;
    do
        c;
    done;
    select y in ;
    do
        d;
    done
}

== stderr 0

== merged 243
f () 
{ 
    for i in "$@";
    do
        :;
    done;
    for i in "$@";
    do
        a;
    done;
    select x in "$@";
    do
        b;
    done;
    for i in ;
    do
        c;
    done;
    select y in ;
    do
        d;
    done
}

== status 0
