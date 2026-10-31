== stdout 303
f () 
{ 
    for ((i = 0; i < 10; i++))
    do
        echo $i;
    done;
    for ((1; 1; 1))
    do
        break;
    done;
    for ((i=0,j=1; i<j; i++, j--))
    do
        :;
    done;
    for ((1; i < 3 ; 1))
    do
        :;
    done;
    for ((i=0; i<3; i++))
    do
        echo $i;
    done
}

== stderr 0

== merged 303
f () 
{ 
    for ((i = 0; i < 10; i++))
    do
        echo $i;
    done;
    for ((1; 1; 1))
    do
        break;
    done;
    for ((i=0,j=1; i<j; i++, j--))
    do
        :;
    done;
    for ((1; i < 3 ; 1))
    do
        :;
    done;
    for ((i=0; i<3; i++))
    do
        echo $i;
    done
}

== status 0
