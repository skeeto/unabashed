== stdout 229
f () 
{ 
    if a; then
        if b; then
            if c; then
                d;
            fi;
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

== merged 229
f () 
{ 
    if a; then
        if b; then
            if c; then
                d;
            fi;
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
