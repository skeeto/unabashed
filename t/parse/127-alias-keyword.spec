== stdout 77
f () 
{ 
    if true; then
        { 
            echo x
        };
    fi
}

== stderr 0

== merged 77
f () 
{ 
    if true; then
        { 
            echo x
        };
    fi
}

== status 0
