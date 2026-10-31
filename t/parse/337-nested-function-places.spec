== stdout 386
f () 
{ 
    if a; then
        function g () 
        { 
            b
        };
    fi;
    { 
        function h () 
        { 
            ( c )
        }
    };
    while :; do
        function k () 
        { 
            d
        };
    done;
    function m () 
    { 
        :
    } | cat;
    function n () 
    { 
        :
    } && function p () 
    { 
        :
    }
}

== stderr 0

== merged 386
f () 
{ 
    if a; then
        function g () 
        { 
            b
        };
    fi;
    { 
        function h () 
        { 
            ( c )
        }
    };
    while :; do
        function k () 
        { 
            d
        };
    done;
    function m () 
    { 
        :
    } | cat;
    function n () 
    { 
        :
    } && function p () 
    { 
        :
    }
}

== status 0
