== stdout 83
f () 
{ 
    a[<(x)]=1;
    echo ${y:-<(z)} "${y:-<(w)}" ${y:->(v)} ${y:-<  (u)}
}

== stderr 0

== merged 83
f () 
{ 
    a[<(x)]=1;
    echo ${y:-<(z)} "${y:-<(w)}" ${y:->(v)} ${y:-<  (u)}
}

== status 0
