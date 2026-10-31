== stdout 85
f () 
{ 
    echo X a > out;
    echo X 2>&1;
    echo X < in > out;
    y x > out
}

== stderr 0

== merged 85
f () 
{ 
    echo X a > out;
    echo X 2>&1;
    echo X < in > out;
    y x > out
}

== status 0
