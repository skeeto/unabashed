== stdout 103
f () 
{ 
    x=$(cat <<EOF
inside
EOF
);
    y=$(cat <<'EOF'
q $x
EOF
);
    echo "$(cat <<A
dq
A
)"
}

== stderr 0

== merged 103
f () 
{ 
    x=$(cat <<EOF
inside
EOF
);
    y=$(cat <<'EOF'
q $x
EOF
);
    echo "$(cat <<A
dq
A
)"
}

== status 0
