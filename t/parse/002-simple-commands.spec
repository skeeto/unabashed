== stdout 54
f () 
{ 
    echo a b c;
    true;
    false;
    :
}

== stderr 0

== merged 54
f () 
{ 
    echo a b c;
    true;
    false;
    :
}

== status 0
