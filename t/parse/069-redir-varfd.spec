== stdout 139
f () 
{ 
    exec {fd}> file;
    exec {fd}< file;
    exec {fd}>&-;
    exec {fd}>&2;
    a {x}>> app;
    a {y}<> rw;
    exec {fd}>&-
}

== stderr 0

== merged 139
f () 
{ 
    exec {fd}> file;
    exec {fd}< file;
    exec {fd}>&-;
    exec {fd}>&2;
    a {x}>> app;
    a {y}<> rw;
    exec {fd}>&-
}

== status 0
