== stdout 212
f () 
{ 
    cat <<A
a
A

    case x in 
        x)
            y
        ;;
    esac
    cat <<B
b
B

    for i in 1;
    do
        :;
    done
    cat <<C
c
C

    [[ -n x ]]
    cat <<D
d
D

    coproc cat
}

== stderr 0

== merged 212
f () 
{ 
    cat <<A
a
A

    case x in 
        x)
            y
        ;;
    esac
    cat <<B
b
B

    for i in 1;
    do
        :;
    done
    cat <<C
c
C

    [[ -n x ]]
    cat <<D
d
D

    coproc cat
}

== status 0
