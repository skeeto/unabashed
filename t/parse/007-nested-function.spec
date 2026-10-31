== stdout 191
outer () 
{ 
    function inner () 
    { 
        echo inner
    };
    function deep () 
    { 
        echo deep
    };
    function sub () 
    { 
        ( echo sub )
    };
    inner
}

== stderr 0

== merged 191
outer () 
{ 
    function inner () 
    { 
        echo inner
    };
    function deep () 
    { 
        echo deep
    };
    function sub () 
    { 
        ( echo sub )
    };
    inner
}

== status 0
