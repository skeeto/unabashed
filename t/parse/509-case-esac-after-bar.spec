== stdout 154
f () 
{ 
    case x in 
        a)

        ;;
        b | esac)
            echo y
        ;;
        c | esac)
            echo z
        ;;
    esac
}

== stderr 0

== merged 154
f () 
{ 
    case x in 
        a)

        ;;
        b | esac)
            echo y
        ;;
        c | esac)
            echo z
        ;;
    esac
}

== status 0
