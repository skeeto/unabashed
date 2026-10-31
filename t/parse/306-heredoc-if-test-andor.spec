== stdout 138
f () 
{ 
    if cat <<A && 
a
A
 b;    then
        c;
    fi;
    if a;
    cat <<B
b
B
    then
        c;
    else
        d;
    fi
}

== stderr 0

== merged 138
f () 
{ 
    if cat <<A && 
a
A
 b;    then
        c;
    fi;
    if a;
    cat <<B
b
B
    then
        c;
    else
        d;
    fi
}

== status 0
