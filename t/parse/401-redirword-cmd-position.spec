== stdout 103
f () 
{ 
    echo a {fd}> x;
    cat {fd}<<< str;
    {a}x > f;
    echo {fd} {"q"} {$v} > x > y > z
}

== stderr 0

== merged 103
f () 
{ 
    echo a {fd}> x;
    cat {fd}<<< str;
    {a}x > f;
    echo {fd} {"q"} {$v} > x > y > z
}

== status 0
