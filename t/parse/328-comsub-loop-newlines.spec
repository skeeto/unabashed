== stdout 138
f () 
{ 
    a=$(for i in a;
do
    b
c;
done);
    b=$(for ((1; 1; 1))
do
    x;
done);
    c=$(select s in "$@";
do
    x; y
z;
done)
}

== stderr 0

== merged 138
f () 
{ 
    a=$(for i in a;
do
    b
c;
done);
    b=$(for ((1; 1; 1))
do
    x;
done);
    c=$(select s in "$@";
do
    x; y
z;
done)
}

== status 0
