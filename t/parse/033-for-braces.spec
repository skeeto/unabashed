== stdout 115
f () 
{ 
    for i in 1 2;
    do
        echo $i;
    done;
    for i in "$@";
    do
        echo $i;
    done
}

== stderr 0

== merged 115
f () 
{ 
    for i in 1 2;
    do
        echo $i;
    done;
    for i in "$@";
    do
        echo $i;
    done
}

== status 0
