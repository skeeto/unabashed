== stdout 64
f () 
{ 
    a 3>&1 1>&2 2>&3 3>&-;
    exec 4<> rw 5>&4 6<&4
}

== stderr 0

== merged 64
f () 
{ 
    a 3>&1 1>&2 2>&3 3>&-;
    exec 4<> rw 5>&4 6<&4
}

== status 0
