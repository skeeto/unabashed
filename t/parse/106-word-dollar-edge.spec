== stdout 50
f () 
{ 
    echo $ "$" a$ $. $/ '$x' \$x "\$x"
}

== stderr 0

== merged 50
f () 
{ 
    echo $ "$" a$ $. $/ '$x' \$x "\$x"
}

== status 0
