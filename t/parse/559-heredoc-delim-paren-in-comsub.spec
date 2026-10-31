== stdout 122
f () 
{ 
    echo $(cat <<')'
x
)
);
    echo $(cat <<`E`
z
`E`
);
    echo "$(cat <<E
w
E
)";
    echo $(cat <<E
v
E
)
}

== stderr 0

== merged 122
f () 
{ 
    echo $(cat <<')'
x
)
);
    echo $(cat <<`E`
z
`E`
);
    echo "$(cat <<E
w
E
)";
    echo $(cat <<E
v
E
)
}

== status 0
