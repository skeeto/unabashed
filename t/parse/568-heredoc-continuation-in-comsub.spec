== stdout 122
f () 
{ 
    echo $(cat <<E
fooE
E
);
    echo $(cat <<'E'
bar\
E
);
    cat <<EX
baz
EX

    echo $(cat <<-E
qux	E
E
)
}

== stderr 0

== merged 122
f () 
{ 
    echo $(cat <<E
fooE
E
);
    echo $(cat <<'E'
bar\
E
);
    cat <<EX
baz
EX

    echo $(cat <<-E
qux	E
E
)
}

== status 0
