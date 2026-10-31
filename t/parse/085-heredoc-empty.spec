== stdout 60
f () 
{ 
    cat <<EOF
EOF

    cat <<''


    echo after
}

== stderr 0

== merged 60
f () 
{ 
    cat <<EOF
EOF

    cat <<''


    echo after
}

== status 0
