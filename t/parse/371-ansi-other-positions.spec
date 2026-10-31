== stdout 222
f () 
{ 
    case 'a	b' in 
        'x	y')

        ;;
        \')

        ;;
    esac;
    [[ $x == '	' && '
' != y ]];
    for i in 'a b' "c";
    do
        :;
    done;
    a > 'o	f' 2>> "e";
    x='v	w' y['k']='z'
}

== stderr 0

== merged 222
f () 
{ 
    case 'a	b' in 
        'x	y')

        ;;
        \')

        ;;
    esac;
    [[ $x == '	' && '
' != y ]];
    for i in 'a b' "c";
    do
        :;
    done;
    a > 'o	f' 2>> "e";
    x='v	w' y['k']='z'
}

== status 0
