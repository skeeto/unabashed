== stdout 108
f () 
{ 
    echo a b c;
    echo joined;
    echo "dq cont";
    echo 'sq \
kept';
    a | b;
    a && b
}

== stderr 0

== merged 108
f () 
{ 
    echo a b c;
    echo joined;
    echo "dq cont";
    echo 'sq \
kept';
    a | b;
    a && b
}

== status 0
