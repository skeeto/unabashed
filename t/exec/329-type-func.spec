== stdout 116
f is a function
f () 
{ 
    echo hi
}
function
status 0
builtin
status 1
function
g is a function
g () 
{ 
    :
}

== stderr 0

== merged 116
f is a function
f () 
{ 
    echo hi
}
function
status 0
builtin
status 1
function
g is a function
g () 
{ 
    :
}

== status 0
