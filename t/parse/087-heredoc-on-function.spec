== stdout 55
f () 
{ 
    cat
} <<EOF
attached to the function
EOF


== stderr 0

== merged 55
f () 
{ 
    cat
} <<EOF
attached to the function
EOF


== status 0
