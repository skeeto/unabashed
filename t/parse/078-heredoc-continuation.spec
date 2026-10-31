== stdout 75
f () 
{ 
    cat <<EOF
joined line
EOF

    cat <<'EOF'
kept \
line
EOF

}

== stderr 0

== merged 75
f () 
{ 
    cat <<EOF
joined line
EOF

    cat <<'EOF'
kept \
line
EOF

}

== status 0
