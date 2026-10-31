== stdout 74
f () 
{ 
    echo ${|REPLY=x; } "${|cmd; }";
    echo ${|REPLY=multi
 }
}

== stderr 0

== merged 74
f () 
{ 
    echo ${|REPLY=x; } "${|cmd; }";
    echo ${|REPLY=multi
 }
}

== status 0
