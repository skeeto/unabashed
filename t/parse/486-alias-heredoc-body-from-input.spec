== stdout 89
f () 
{ 
    cat <<EOF
line one
EOF

    cat <<A
body
A

    echo mid x
    echo after
}

== stderr 0

== merged 89
f () 
{ 
    cat <<EOF
line one
EOF

    cat <<A
body
A

    echo mid x
    echo after
}

== status 0
