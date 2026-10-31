== stdout 129
f () 
{ 
    cat <(cat <(echo deep));
    cat <(echo $(cmd)) "$(cat <(x))";
    cat <(case x in x)
        echo y
    ;;
esac)
}

== stderr 0

== merged 129
f () 
{ 
    cat <(cat <(echo deep));
    cat <(echo $(cmd)) "$(cat <(x))";
    cat <(case x in x)
        echo y
    ;;
esac)
}

== status 0
