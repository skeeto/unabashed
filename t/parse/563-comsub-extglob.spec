== stdout 271
f () 
{ 
    echo $(echo @(a|b) !(c) +(d|e)f *(g) ?(h));
    echo $(case x in @(a|b))
        echo 1
    ;;
    !(*\)*))
        echo 2
    ;;
esac);
    echo "$(ls +([0-9]).txt)";
    echo `echo @(x|y)`;
    echo ${ echo *(z); };
    [[ $(echo a) == @(a|$(echo b)) ]]
}

== stderr 0

== merged 271
f () 
{ 
    echo $(echo @(a|b) !(c) +(d|e)f *(g) ?(h));
    echo $(case x in @(a|b))
        echo 1
    ;;
    !(*\)*))
        echo 2
    ;;
esac);
    echo "$(ls +([0-9]).txt)";
    echo `echo @(x|y)`;
    echo ${ echo *(z); };
    [[ $(echo a) == @(a|$(echo b)) ]]
}

== status 0
