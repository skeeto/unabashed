== stdout 92
f () 
{ 
    echo $(a $(b $(c)));
    echo "$(a "$(b "$(c)")")";
    echo $(echo `inner`)
}

== stderr 0

== merged 92
f () 
{ 
    echo $(a $(b $(c)));
    echo "$(a "$(b "$(c)")")";
    echo $(echo `inner`)
}

== status 0
