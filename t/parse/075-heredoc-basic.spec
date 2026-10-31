== stdout 44
f () 
{ 
    cat <<EOF
hello
  world
EOF

}

== stderr 0

== merged 44
f () 
{ 
    cat <<EOF
hello
  world
EOF

}

== status 0
