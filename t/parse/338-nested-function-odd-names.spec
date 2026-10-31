== stdout 284
f () 
{ 
    function a-b () 
    { 
        :
    };
    function x=y () 
    { 
        :
    };
    function 9 () 
    { 
        :
    };
    function g () 
    { 
        for i in 1;
        do
            :;
        done
    };
    function h () 
    { 
        (( 1 ))
    }
}

== stderr 0

== merged 284
f () 
{ 
    function a-b () 
    { 
        :
    };
    function x=y () 
    { 
        :
    };
    function 9 () 
    { 
        :
    };
    function g () 
    { 
        for i in 1;
        do
            :;
        done
    };
    function h () 
    { 
        (( 1 ))
    }
}

== status 0
