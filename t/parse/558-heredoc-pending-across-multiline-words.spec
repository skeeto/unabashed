== stdout 216
f () 
{ 
    cat <<A
abody
A

    echo $(echo x
echo y)
    cat <<B
bbody
B

    echo "q
r"
    cat <<C |
cbody
C
  cat
    cat <<D && 
dbody
D
 { 
        echo in
    };
    cat <<F
fbody
F

    echo ${ echo g
 }
}

== stderr 0

== merged 216
f () 
{ 
    cat <<A
abody
A

    echo $(echo x
echo y)
    cat <<B
bbody
B

    echo "q
r"
    cat <<C |
cbody
C
  cat
    cat <<D && 
dbody
D
 { 
        echo in
    };
    cat <<F
fbody
F

    echo ${ echo g
 }
}

== status 0
