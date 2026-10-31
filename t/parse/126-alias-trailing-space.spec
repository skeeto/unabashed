== stdout 43
f () 
{ 
    nice ls -l x;
    nice ls y
}

== stderr 0

== merged 43
f () 
{ 
    nice ls -l x;
    nice ls y
}

== status 0
