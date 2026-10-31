== stdout 93
f () 
{ 
    cat <<A > out <<-B 2>&1 <<'C' {v}<<D 3<<< str
a
A
b
B
c
C
d
D

    echo after
}

== stderr 0

== merged 93
f () 
{ 
    cat <<A > out <<-B 2>&1 <<'C' {v}<<D 3<<< str
a
A
b
B
c
C
d
D

    echo after
}

== status 0
