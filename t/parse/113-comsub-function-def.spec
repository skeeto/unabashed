== stdout 90
f () 
{ 
    x=$(function g () 
{ 
    echo g
}; g);
    y=$(function h () 
{ 
    :
})
}

== stderr 0

== merged 90
f () 
{ 
    x=$(function g () 
{ 
    echo g
}; g);
    y=$(function h () 
{ 
    :
})
}

== status 0
