== stdout 187
f () 
{ 
    case $x in 
        a)
            cat <<A
a
A

        ;&
        b)
            cat <<B |
b
B
  wc
        ;;&
        c)
            cat <<C &
c
C
 
        ;;
    esac
}

== stderr 0

== merged 187
f () 
{ 
    case $x in 
        a)
            cat <<A
a
A

        ;&
        b)
            cat <<B |
b
B
  wc
        ;;&
        c)
            cat <<C &
c
C
 
        ;;
    esac
}

== status 0
