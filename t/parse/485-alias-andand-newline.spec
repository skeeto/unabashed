== stdout 50
f () 
{ 
    true && echo x;
    true || echo y
}

== stderr 0

== merged 50
f () 
{ 
    true && echo x;
    true || echo y
}

== status 0
