== stdout 115
f () 
{ 
    cat <<EOF && 
body
EOF
 echo ok || echo no
    cat <<EOF &
bg
EOF
  cat <<EOF > out 2>&1
redir
EOF

}

== stderr 0

== merged 115
f () 
{ 
    cat <<EOF && 
body
EOF
 echo ok || echo no
    cat <<EOF &
bg
EOF
  cat <<EOF > out 2>&1
redir
EOF

}

== status 0
