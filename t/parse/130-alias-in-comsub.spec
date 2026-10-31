== stdout 61
f () 
{ 
    x=$(q one);
    y=`q two`;
    z="$(q three)"
}

== stderr 0

== merged 61
f () 
{ 
    x=$(q one);
    y=`q two`;
    z="$(q three)"
}

== status 0
