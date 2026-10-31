== stdout 89
f () 
{ 
    cat <<-EOF
tabbed
  mixed
    spaces kept
EOF

    cat <<-'Q'
$literal
Q

}

== stderr 0

== merged 89
f () 
{ 
    cat <<-EOF
tabbed
  mixed
    spaces kept
EOF

    cat <<-'Q'
$literal
Q

}

== status 0
