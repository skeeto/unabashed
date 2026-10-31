== stdout 114
f () 
{ 
    cat <<END_1
a
END_1

    cat <<'with space'
b
with space

    cat <<-'-'
c
-

    cat <<'$x'
d
$x

}

== stderr 0

== merged 114
f () 
{ 
    cat <<END_1
a
END_1

    cat <<'with space'
b
with space

    cat <<-'-'
c
-

    cat <<'$x'
d
$x

}

== status 0
