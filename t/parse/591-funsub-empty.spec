== stdout 87
f () 
{ 
    echo ${ };
    echo ${ };
    echo ${ };
    echo "${ }";
    echo ${ }
}

== stderr 0

== merged 87
f () 
{ 
    echo ${ };
    echo ${ };
    echo ${ };
    echo "${ }";
    echo ${ }
}

== status 0
