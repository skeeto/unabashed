== stdout 97
f () 
{ 
    { 
        read a;
        read b
    } <<EOF
1
2
EOF

    ( cat ) <<EOF
sub
EOF

}

== stderr 0

== merged 97
f () 
{ 
    { 
        read a;
        read b
    } <<EOF
1
2
EOF

    ( cat ) <<EOF
sub
EOF

}

== status 0
