== stdout 166
f () 
{ 
    for x in "$@";
    do
        echo $x;
    done;
    select y in "$@";
    do
        echo $y;
    done;
    for z in "$@";
    do
        :;
    done
}

== stderr 0

== merged 166
f () 
{ 
    for x in "$@";
    do
        echo $x;
    done;
    select y in "$@";
    do
        echo $y;
    done;
    for z in "$@";
    do
        :;
    done
}

== status 0
