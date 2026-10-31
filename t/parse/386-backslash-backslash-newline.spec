== stdout 56
f () 
{ 
    echo a\\;
    echo b \\c "d\\
e" 'f\\
g'
}

== stderr 0

== merged 56
f () 
{ 
    echo a\\;
    echo b \\c "d\\
e" 'f\\
g'
}

== status 0
