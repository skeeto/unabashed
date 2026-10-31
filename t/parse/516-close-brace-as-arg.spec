== stdout 189
f () 
{ 
    echo }
}
g () 
{ 
    { 
        echo
    }
}
h () 
{ 
    echo a } b;
    echo }};
    x=};
    echo {}
}
i () 
{ 
    if a; then
        { 
            b
        };
    fi
}

== stderr 0

== merged 189
f () 
{ 
    echo }
}
g () 
{ 
    { 
        echo
    }
}
h () 
{ 
    echo a } b;
    echo }};
    x=};
    echo {}
}
i () 
{ 
    if a; then
        { 
            b
        };
    fi
}

== status 0
