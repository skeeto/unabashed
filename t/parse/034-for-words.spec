== stdout 182
f () 
{ 
    for i in "$@" *.c ${arr[@]} $(ls) {1..3} 'q w';
    do
        :;
    done;
    for in in in;
    do
        :;
    done;
    for do in do;
    do
        :;
    done
}

== stderr 0

== merged 182
f () 
{ 
    for i in "$@" *.c ${arr[@]} $(ls) {1..3} 'q w';
    do
        :;
    done;
    for in in in;
    do
        :;
    done;
    for do in do;
    do
        :;
    done
}

== status 0
