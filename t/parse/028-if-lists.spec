== stdout 178
f () 
{ 
    if a && b;
    c || d; then
        e;
        g;
    fi;
    if ! a | b; then
        :;
    fi;
    if { 
        a;
        b
    }; then
        ( c );
    fi
}

== stderr 0

== merged 178
f () 
{ 
    if a && b;
    c || d; then
        e;
        g;
    fi;
    if ! a | b; then
        :;
    fi;
    if { 
        a;
        b
    }; then
        ( c );
    fi
}

== status 0
