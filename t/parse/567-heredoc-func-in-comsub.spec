== stdout 188
f () 
{ 
    x=$(function g () 
{ 
    cat <<E
x
E

}; g);
    y=$(function h () 
{ 
    :
} <<E
y
E
);
    z=$(function k () 
{ 
    cat
} <<E 2>&1
z
E
);
    cat <<< "$(cat <<E
w
E
)"
}

== stderr 0

== merged 188
f () 
{ 
    x=$(function g () 
{ 
    cat <<E
x
E

}; g);
    y=$(function h () 
{ 
    :
} <<E
y
E
);
    z=$(function k () 
{ 
    cat
} <<E 2>&1
z
E
);
    cat <<< "$(cat <<E
w
E
)"
}

== status 0
