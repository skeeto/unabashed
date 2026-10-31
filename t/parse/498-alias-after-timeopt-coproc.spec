== stdout 121
f () 
{ 
    time -p echo X;
    time -p echo X;
    echo X;
    time ! echo X;
    coproc echo X;
    a 2>&1 | echo X
}

== stderr 0

== merged 121
f () 
{ 
    time -p echo X;
    time -p echo X;
    echo X;
    time ! echo X;
    coproc echo X;
    a 2>&1 | echo X
}

== status 0
