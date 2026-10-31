== stdout 74
f () 
{ 
    function g () 
    { 
        echo g
    };
    echo after
}

== stderr 0

== merged 74
f () 
{ 
    function g () 
    { 
        echo g
    };
    echo after
}

== status 0
