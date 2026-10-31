== stdout 96
f () 
{ 
    coproc cmd 2>&1;
    coproc C { 
        cmd
    } > out;
    coproc cmd | other
}

== stderr 0

== merged 96
f () 
{ 
    coproc cmd 2>&1;
    coproc C { 
        cmd
    } > out;
    coproc cmd | other
}

== status 0
