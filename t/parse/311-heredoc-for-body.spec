== stdout 203
f () 
{ 
    for i in a b;
    do
        cat <<A
$i
A

    done
    for ((i=0; i<2; i++))
    do
        cat <<A
x
A

        echo $i;
    done
    select s in a;
    do
        cat <<S
s
S

    done
}

== stderr 0

== merged 203
f () 
{ 
    for i in a b;
    do
        cat <<A
$i
A

    done
    for ((i=0; i<2; i++))
    do
        cat <<A
x
A

        echo $i;
    done
    select s in a;
    do
        cat <<S
s
S

    done
}

== status 0
