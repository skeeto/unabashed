== stdout 55
f () 
{ 
    cat <<EOF
$(echo "a)b")
${x//\}/y}
EOF

}

== stderr 0

== merged 55
f () 
{ 
    cat <<EOF
$(echo "a)b")
${x//\}/y}
EOF

}

== status 0
