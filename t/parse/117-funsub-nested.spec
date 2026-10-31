== stdout 165
f () 
{ 
    echo ${ echo $(inner); } $(echo ${ inner; });
    echo ${ case x in x)
        echo y
    ;;
esac; };
    echo ${ for i in 1;
do
    echo $i;
done; }
}

== stderr 0

== merged 165
f () 
{ 
    echo ${ echo $(inner); } $(echo ${ inner; });
    echo ${ case x in x)
        echo y
    ;;
esac; };
    echo ${ for i in 1;
do
    echo $i;
done; }
}

== status 0
