== stdout 60
f () 
{ 
    x=$(echo "a # )" '# )');
    y=$(a | b && c)
}

== stderr 0

== merged 60
f () 
{ 
    x=$(echo "a # )" '# )');
    y=$(a | b && c)
}

== status 0
