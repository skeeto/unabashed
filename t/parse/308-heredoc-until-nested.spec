== stdout 204
f () 
{ 
    for i in 1;
    do
        case $i in 
            1)
                until cat <<U
u
U
                do
                    :;
                done
            ;;
        esac;
    done
}

== stderr 0

== merged 204
f () 
{ 
    for i in 1;
    do
        case $i in 
            1)
                until cat <<U
u
U
                do
                    :;
                done
            ;;
        esac;
    done
}

== status 0
