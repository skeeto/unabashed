== stdout 86
f () 
{ 
    a=${ q 1; };
    b=${|q 2; };
    cat <(q 3) >(q 4);
    c=$( ( q 5 ))
}

== stderr 0

== merged 86
f () 
{ 
    a=${ q 1; };
    b=${|q 2; };
    cat <(q 3) >(q 4);
    c=$( ( q 5 ))
}

== status 0
