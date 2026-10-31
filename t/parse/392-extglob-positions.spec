== stdout 140
f () 
{ 
    @(a) arg;
    x=+(b) y;
    for i in ?(a) *(b);
    do
        :;
    done;
    a=(@(c) !(d));
    echo ${x/@(e|f)/g} > @(h)
}

== stderr 0

== merged 140
f () 
{ 
    @(a) arg;
    x=+(b) y;
    for i in ?(a) *(b);
    do
        :;
    done;
    a=(@(c) !(d));
    echo ${x/@(e|f)/g} > @(h)
}

== status 0
