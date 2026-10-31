== stdout 213
f () 
{ 
    case $x in 
        a)
            cat <<A
a
A

            echo after
        ;;
        b)
            echo b
            cat <<B
b
B

        ;;
        c)
            echo c
        ;;
    esac
}

== stderr 0

== merged 213
f () 
{ 
    case $x in 
        a)
            cat <<A
a
A

            echo after
        ;;
        b)
            echo b
            cat <<B
b
B

        ;;
        c)
            echo c
        ;;
    esac
}

== status 0
