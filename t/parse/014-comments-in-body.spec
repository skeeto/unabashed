== stdout 79
f () 
{ 
    echo a;
    echo b#notcomment c;
    echo '#quoted' "#dq" \#esc
}

== stderr 0

== merged 79
f () 
{ 
    echo a;
    echo b#notcomment c;
    echo '#quoted' "#dq" \#esc
}

== status 0
