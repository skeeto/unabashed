== stdout 209
f () 
{ 
    case x in 
    esac
}
g () 
{ 
    case x in 
    esac
}
h () 
{ 
    case x in 
        a)

        ;;
        b)

        ;;
    esac
}
i () 
{ 
    case x in 
        a)

        ;;
    esac
}

== stderr 0

== merged 209
f () 
{ 
    case x in 
    esac
}
g () 
{ 
    case x in 
    esac
}
h () 
{ 
    case x in 
        a)

        ;;
        b)

        ;;
    esac
}
i () 
{ 
    case x in 
        a)

        ;;
    esac
}

== status 0
