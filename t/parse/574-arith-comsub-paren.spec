== stdout 90
f () 
{ 
    echo $(( $(echo ")") ));
    echo $(( `echo ")"` ));
    (( $(echo ")") ))
}

== stderr 0

== merged 90
f () 
{ 
    echo $(( $(echo ")") ));
    echo $(( `echo ")"` ));
    (( $(echo ")") ))
}

== status 0
