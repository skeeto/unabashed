== stdout 150
f () 
{ 
    for x in "$@";
    do
        :;
    done;
    for y in a b;
    do
        :;
    done;
    for z in "$@";
    do
        :;
    done
}

== stderr 0

== merged 150
f () 
{ 
    for x in "$@";
    do
        :;
    done;
    for y in a b;
    do
        :;
    done;
    for z in "$@";
    do
        :;
    done
}

== status 0
