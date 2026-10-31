== stdout 61
g () 
{ 
    if cat <<E &
x
E
 ;    then
        :;
    fi
}

== stderr 0

== merged 61
g () 
{ 
    if cat <<E &
x
E
 ;    then
        :;
    fi
}

== status 0
