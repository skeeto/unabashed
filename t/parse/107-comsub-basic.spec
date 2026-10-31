== stdout 123
f () 
{ 
    echo $(cmd) $(cmd arg | other) $(spaced) $(a; b) $(a && b || c);
    echo "$(cmd)" "x$(cmd)y" $(multi
line)
}

== stderr 0

== merged 123
f () 
{ 
    echo $(cmd) $(cmd arg | other) $(spaced) $(a; b) $(a && b || c);
    echo "$(cmd)" "x$(cmd)y" $(multi
line)
}

== status 0
