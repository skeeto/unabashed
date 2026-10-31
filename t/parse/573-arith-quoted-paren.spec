== stdout 64
f () 
{ 
    echo $(( ')' + ")" ));
    echo $(( ${x:-')'} ))
}

== stderr 0

== merged 64
f () 
{ 
    echo $(( ')' + ")" ));
    echo $(( ${x:-')'} ))
}

== status 0
