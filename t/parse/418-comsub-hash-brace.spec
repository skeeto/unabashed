== stdout 91
f () 
{ 
    echo $(echo ${x#*}) $(echo ${#x}#) "$(echo a)" $(case x in x)

    ;;
esac)
}

== stderr 0

== merged 91
f () 
{ 
    echo $(echo ${x#*}) $(echo ${#x}#) "$(echo a)" $(case x in x)

    ;;
esac)
}

== status 0
