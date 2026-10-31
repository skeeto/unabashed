== stdout 337
f () 
{ 
    case x in 
        a)

        ;;
        b)

        ;&
        c)

        ;;&
    esac;
    case x in 
        a | b)

        ;;
    esac;
    case x in 
        a)

        ;;
    esac;
    case x in 
        a)
            b
        ;;
        c)

        ;;
    esac;
    case x in 
        a)

        ;;
    esac
}

== stderr 0

== merged 337
f () 
{ 
    case x in 
        a)

        ;;
        b)

        ;&
        c)

        ;;&
    esac;
    case x in 
        a | b)

        ;;
    esac;
    case x in 
        a)

        ;;
    esac;
    case x in 
        a)
            b
        ;;
        c)

        ;;
    esac;
    case x in 
        a)

        ;;
    esac
}

== status 0
