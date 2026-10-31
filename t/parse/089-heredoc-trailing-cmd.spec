== stdout 91
f () 
{ 
    cat <<EOF
body
EOF

    echo after
    cat <<EOF |
x
EOF
  cat
    echo two
}

== stderr 0

== merged 91
f () 
{ 
    cat <<EOF
body
EOF

    echo after
    cat <<EOF |
x
EOF
  cat
    echo two
}

== status 0
