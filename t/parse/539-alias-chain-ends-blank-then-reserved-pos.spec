== stdout 71
f () 
{ 
    echo X;
    echo Y;
    if true; then
        :;
    fi
}

== stderr 0

== merged 71
f () 
{ 
    echo X;
    echo Y;
    if true; then
        :;
    fi
}

== status 0
