== stdout 118
f () 
{ 
    if true; then
        :;
    fi;
    if true; then
        echo a;
    fi;
    { 
        echo c
    }
}

== stderr 0

== merged 118
f () 
{ 
    if true; then
        :;
    fi;
    if true; then
        echo a;
    fi;
    { 
        echo c
    }
}

== status 0
