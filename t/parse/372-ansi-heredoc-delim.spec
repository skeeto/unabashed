== stdout 59
f () 
{ 
    cat <<'EA'
body $x
EA

    cat <<'Q'
two
Q

}

== stderr 0

== merged 59
f () 
{ 
    cat <<'EA'
body $x
EA

    cat <<'Q'
two
Q

}

== status 0
