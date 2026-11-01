== stdout 159
[f says one]
[f says two]
[g says a]
[g says b]
[h x]
[h err]
f () 
{ 
    echo "f says $1"
} >> /tmp/f
g () 
{ 
    echo "g says $1"
} > $out
read f says one

== stderr 0

== merged 159
[f says one]
[f says two]
[g says a]
[g says b]
[h x]
[h err]
f () 
{ 
    echo "f says $1"
} >> /tmp/f
g () 
{ 
    echo "g says $1"
} > $out
read f says one

== status 0
