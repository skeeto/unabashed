== stdout 844
f () 
{ 
    if true; then
        while read a; do
            for i in 1 2;
            do
                until false; do
                    { 
                        echo $i
                    } 1>&2 2> /dev/null;
                    case $i in 
                        1)
                            ( echo one ) >> log
                        ;;
                    esac 3< in;
                    select s in a;
                    do
                        break;
                    done 0<&0;
                done 4> out;
            done <<E
heredoc
E

        done < input 2>&1;
    else
        if false; then
            [[ -n x ]] > /dev/null;
            (( 1 )) 2> /dev/null;
        else
            function f2 () 
            { 
                echo
            } > f2out;
        fi;
    fi > outer 2>> err <<< "str"
}

== stderr 0

== merged 844
f () 
{ 
    if true; then
        while read a; do
            for i in 1 2;
            do
                until false; do
                    { 
                        echo $i
                    } 1>&2 2> /dev/null;
                    case $i in 
                        1)
                            ( echo one ) >> log
                        ;;
                    esac 3< in;
                    select s in a;
                    do
                        break;
                    done 0<&0;
                done 4> out;
            done <<E
heredoc
E

        done < input 2>&1;
    else
        if false; then
            [[ -n x ]] > /dev/null;
            (( 1 )) 2> /dev/null;
        else
            function f2 () 
            { 
                echo
            } > f2out;
        fi;
    fi > outer 2>> err <<< "str"
}

== status 0
