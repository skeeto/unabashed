== stdout 134
hi ann, n=1
hi bob, n=2
item-a
item-b
item-c
greet () 
{ 
    while read l; do
        echo "$l";
    done <<EOF2
hi $1, n=$n
EOF2

}

== stderr 0

== merged 134
hi ann, n=1
hi bob, n=2
item-a
item-b
item-c
greet () 
{ 
    while read l; do
        echo "$l";
    done <<EOF2
hi $1, n=$n
EOF2

}

== status 0
