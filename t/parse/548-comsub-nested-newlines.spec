== stdout 121
f () 
{ 
    echo $(echo $(echo $(echo a)
echo b) $(echo c));
    x=$($($(echo d)));
    echo "$(echo "$(echo "e
")")"
}

== stderr 0

== merged 121
f () 
{ 
    echo $(echo $(echo $(echo a)
echo b) $(echo c));
    x=$($($(echo d)));
    echo "$(echo "$(echo "e
")")"
}

== status 0
