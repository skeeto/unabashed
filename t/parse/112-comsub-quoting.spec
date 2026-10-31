== stdout 121
f () 
{ 
    echo $(echo ')') $(echo ")") $(echo \)) $(echo "$(echo "(")");
    echo $(echo 'it'\''s') "$(echo "a b")"
}

== stderr 0

== merged 121
f () 
{ 
    echo $(echo ')') $(echo ")") $(echo \)) $(echo "$(echo "(")");
    echo $(echo 'it'\''s') "$(echo "a b")"
}

== status 0
