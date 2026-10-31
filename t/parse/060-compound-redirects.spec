== stdout 422
f () 
{ 
    while read l; do
        echo $l;
    done < in;
    { 
        a;
        b
    } > out 2>&1;
    ( a ) 2> err;
    if a; then
        b;
    fi > out;
    case x in 
        x)

        ;;
    esac 2> /dev/null;
    for i in 1;
    do
        :;
    done | cat;
    until a; do
        b;
    done >> log;
    [[ -n a ]] > out;
    (( x )) 2> err;
    select x in a;
    do
        :;
    done < /dev/tty
}

== stderr 0

== merged 422
f () 
{ 
    while read l; do
        echo $l;
    done < in;
    { 
        a;
        b
    } > out 2>&1;
    ( a ) 2> err;
    if a; then
        b;
    fi > out;
    case x in 
        x)

        ;;
    esac 2> /dev/null;
    for i in 1;
    do
        :;
    done | cat;
    until a; do
        b;
    done >> log;
    [[ -n a ]] > out;
    (( x )) 2> err;
    select x in a;
    do
        :;
    done < /dev/tty
}

== status 0
