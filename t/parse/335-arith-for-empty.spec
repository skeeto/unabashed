== stdout 379
f () 
{ 
    for ((i=0; 1; 1))
    do
        :;
    done;
    for ((1; i<3; 1))
    do
        :;
    done;
    for ((1; 1; i++ ))
    do
        :;
    done;
    for ((1; 1; 1))
    do
        :;
    done;
    for ((i=0, j=0; i<3; i++, j++))
    do
        :;
    done;
    for ((x; y; z))
    do
        :;
    done;
    for ((
i=0; 
i<3; 
i++
))
    do
        :;
    done
}

== stderr 0

== merged 379
f () 
{ 
    for ((i=0; 1; 1))
    do
        :;
    done;
    for ((1; i<3; 1))
    do
        :;
    done;
    for ((1; 1; i++ ))
    do
        :;
    done;
    for ((1; 1; 1))
    do
        :;
    done;
    for ((i=0, j=0; i<3; i++, j++))
    do
        :;
    done;
    for ((x; y; z))
    do
        :;
    done;
    for ((
i=0; 
i<3; 
i++
))
    do
        :;
    done
}

== status 0
