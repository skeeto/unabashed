== stdout 58
f () 
{ 
    echo "a b";
    echo "a
c";
    echo 'b d'
}

== stderr 0

== merged 58
f () 
{ 
    echo "a b";
    echo "a
c";
    echo 'b d'
}

== status 0
