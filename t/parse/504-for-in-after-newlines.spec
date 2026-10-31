== stdout 147
f () 
{ 
    for x in a b;
    do
        :;
    done;
    for y in c;
    do
        :;
    done;
    select z in d;
    do
        :;
    done
}

== stderr 0

== merged 147
f () 
{ 
    for x in a b;
    do
        :;
    done;
    for y in c;
    do
        :;
    done;
    select z in d;
    do
        :;
    done
}

== status 0
