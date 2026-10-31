== stdout 218
f () 
{ 
    case "$1" in 
        a)

        ;;
    esac;
    case ${x:-y} in 
        a)

        ;;
    esac;
    case $(cmd) in 
        a)

        ;;
    esac;
    case in in 
        in)

        ;;
    esac
}

== stderr 0

== merged 218
f () 
{ 
    case "$1" in 
        a)

        ;;
    esac;
    case ${x:-y} in 
        a)

        ;;
    esac;
    case $(cmd) in 
        a)

        ;;
    esac;
    case in in 
        in)

        ;;
    esac
}

== status 0
