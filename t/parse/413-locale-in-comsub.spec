== stdout 72
f () 
{ 
    echo "$(echo "x" 'y')" $(echo "$"z"") "a\$b" "`c`" "a'b"
}

== stderr 0

== merged 72
f () 
{ 
    echo "$(echo "x" 'y')" $(echo "$"z"") "a\$b" "`c`" "a'b"
}

== status 0
