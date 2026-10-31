== stdout 126
f () 
{ 
    cat <<'EOF'
$x `y` $(z)
EOF

    cat <<'EOF'
$x \$y
EOF

    cat <<'EOF'
$x
EOF

    cat <<'EF'
literal $x
EF

}

== stderr 0

== merged 126
f () 
{ 
    cat <<'EOF'
$x `y` $(z)
EOF

    cat <<'EOF'
$x \$y
EOF

    cat <<'EOF'
$x
EOF

    cat <<'EF'
literal $x
EF

}

== status 0
