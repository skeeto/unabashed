== stdout 69
f () 
{ 
    echo a || echo c;
    echo b >> out;
    echo a | cat
}

== stderr 0

== merged 69
f () 
{ 
    echo a || echo c;
    echo b >> out;
    echo a | cat
}

== status 0
