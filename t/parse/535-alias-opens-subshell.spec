== stdout 57
f () 
{ 
    ( echo in );
    { 
        echo in
    }
}

== stderr 0

== merged 57
f () 
{ 
    ( echo in );
    { 
        echo in
    }
}

== status 0
