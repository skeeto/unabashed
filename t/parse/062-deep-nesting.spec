== stdout 480
f () 
{ 
    for i in 1 2;
    do
        while a; do
            case $i in 
                1)
                    if b; then
                        { 
                            c;
                            ( d | e ) && [[ -n x ]]
                        };
                    else
                        until g; do
                            (( n++ ));
                        done;
                    fi
                ;;
            esac;
        done;
    done
}

== stderr 0

== merged 480
f () 
{ 
    for i in 1 2;
    do
        while a; do
            case $i in 
                1)
                    if b; then
                        { 
                            c;
                            ( d | e ) && [[ -n x ]]
                        };
                    else
                        until g; do
                            (( n++ ));
                        done;
                    fi
                ;;
            esac;
        done;
    done
}

== status 0
