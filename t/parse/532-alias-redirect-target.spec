== stdout 62
f () 
{ 
    echo > x;
    y > x;
    y <<< x;
    y x 2>&1
}

== stderr 0

== merged 62
f () 
{ 
    echo > x;
    y > x;
    y <<< x;
    y x 2>&1
}

== status 0
