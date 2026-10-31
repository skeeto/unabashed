== stdout 51
f () 
{ 
    x=1 echo Y;
    x=1;
    x=2 echo Y
}

== stderr 0

== merged 51
f () 
{ 
    x=1 echo Y;
    x=1;
    x=2 echo Y
}

== status 0
