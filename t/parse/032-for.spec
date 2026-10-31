== stdout 274
f () 
{ 
    for i in a b c;
    do
        echo $i;
    done;
    for i in "$@";
    do
        echo $i;
    done;
    for i in ;
    do
        echo never;
    done;
    for i in "$@";
    do
        echo $i;
    done;
    for i in a b;
    do
        echo $i;
    done
}

== stderr 0

== merged 274
f () 
{ 
    for i in a b c;
    do
        echo $i;
    done;
    for i in "$@";
    do
        echo $i;
    done;
    for i in ;
    do
        echo never;
    done;
    for i in "$@";
    do
        echo $i;
    done;
    for i in a b;
    do
        echo $i;
    done
}

== status 0
