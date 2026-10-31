== stdout 46
f () 
{ 
    cat <<EOF
 EOF
EOF2
EOF x
EOF

}

== stderr 0

== merged 46
f () 
{ 
    cat <<EOF
 EOF
EOF2
EOF x
EOF

}

== status 0
