== stdout 285
f () 
{ 
    time -p ! a | b;
    time -p ! { 
        a
    };
    time ( a ) > out;
    time -p a;
    time -p b;
    time -p -p c;
    time -p ! d;
    time -p ;
    time -p ;
    time a;
    time a;
    time while a; do
        b;
    done;
    if time ; then
        ! ;
    fi
}

== stderr 0

== merged 285
f () 
{ 
    time -p ! a | b;
    time -p ! { 
        a
    };
    time ( a ) > out;
    time -p a;
    time -p b;
    time -p -p c;
    time -p ! d;
    time -p ;
    time -p ;
    time a;
    time a;
    time while a; do
        b;
    done;
    if time ; then
        ! ;
    fi
}

== status 0
