== stdout 37
f () 
{ 
    x=`cat <<EOF
bq
EOF
`
}

== stderr 0

== merged 37
f () 
{ 
    x=`cat <<EOF
bq
EOF
`
}

== status 0
