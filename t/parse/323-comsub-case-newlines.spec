== stdout 125
f () 
{ 
    x=$(case a in a)
        b
c
    ;;
    d)
        e
    ;;
esac);
    y=$(case a in a)

    ;;
esac; echo z)
}

== stderr 0

== merged 125
f () 
{ 
    x=$(case a in a)
        b
c
    ;;
    d)
        e
    ;;
esac);
    y=$(case a in a)

    ;;
esac; echo z)
}

== status 0
