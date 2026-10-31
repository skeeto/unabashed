== stdout 106
f () 
{ 
    a > out;
    a < in;
    a >> app;
    a >| clob;
    a <> rw;
    a 2> err;
    a 2>> err
}

== stderr 0

== merged 106
f () 
{ 
    a > out;
    a < in;
    a >> app;
    a >| clob;
    a <> rw;
    a 2> err;
    a 2>> err
}

== status 0
