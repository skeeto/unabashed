== stdout 54
f () 
{ 
    x=$(echo a);
    y=$(echo '#' "#" a#b)
}

== stderr 0

== merged 54
f () 
{ 
    x=$(echo a);
    y=$(echo '#' "#" a#b)
}

== status 0
