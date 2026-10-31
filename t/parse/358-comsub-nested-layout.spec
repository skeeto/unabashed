== stdout 102
f () 
{ 
    a=$(x $(y
z) $(if p; then
    q;
fi));
    b="$(echo "$(x; y)")";
    c=$(echo `x;
y`)
}

== stderr 0

== merged 102
f () 
{ 
    a=$(x $(y
z) $(if p; then
    q;
fi));
    b="$(echo "$(x; y)")";
    c=$(echo `x;
y`)
}

== status 0
