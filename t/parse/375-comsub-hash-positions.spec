== stdout 76
f () 
{ 
    echo $(echo a#b c#) $(echo ${#}#) $(echo x) $(echo '#'z \#w)
}

== stderr 0

== merged 76
f () 
{ 
    echo $(echo a#b c#) $(echo ${#}#) $(echo x) $(echo '#'z \#w)
}

== status 0
