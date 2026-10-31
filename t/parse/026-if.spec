== stdout 294
f () 
{ 
    if a; then
        b;
    fi;
    if a; then
        b;
    else
        c;
    fi;
    if a; then
        b;
    else
        if c; then
            d;
        else
            if e; then
                g;
            else
                h;
            fi;
        fi;
    fi
}

== stderr 0

== merged 294
f () 
{ 
    if a; then
        b;
    fi;
    if a; then
        b;
    else
        c;
    fi;
    if a; then
        b;
    else
        if c; then
            d;
        else
            if e; then
                g;
            else
                h;
            fi;
        fi;
    fi
}

== status 0
