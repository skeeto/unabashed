== stdout 151
f () 
{ 
    (( a )) && (( b )) || ((c));
    ! (( a ));
    while (( i < 3 )); do
        (( i++ ));
    done;
    if ((x)); then
        :;
    fi
}

== stderr 0

== merged 151
f () 
{ 
    (( a )) && (( b )) || ((c));
    ! (( a ));
    while (( i < 3 )); do
        (( i++ ));
    done;
    if ((x)); then
        :;
    fi
}

== status 0
