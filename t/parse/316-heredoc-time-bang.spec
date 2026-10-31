== stdout 116
f () 
{ 
    time cat <<A |
a
A
  wc
    ! cat <<B
b
B

    time -p ! { 
        cat <<C
c
C

    };
    echo end
}

== stderr 0

== merged 116
f () 
{ 
    time cat <<A |
a
A
  wc
    ! cat <<B
b
B

    time -p ! { 
        cat <<C
c
C

    };
    echo end
}

== status 0
