== stdout 138
f () 
{ 
    if a; then
        b;
        c;
    else
        if d; then
            e;
        else
            g;
        fi;
    fi
}

== stderr 0

== merged 138
f () 
{ 
    if a; then
        b;
        c;
    else
        if d; then
            e;
        else
            g;
        fi;
    fi
}

== status 0
