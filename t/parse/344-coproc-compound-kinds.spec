== stdout 277
f () 
{ 
    coproc X if a; then
        b;
    fi;
    coproc COPROC for i in 1;
    do
        :;
    done;
    coproc COPROC [[ -n x ]];
    coproc COPROC (( x ));
    coproc N (( x ));
    coproc N [[ -n y ]];
    coproc COPROC case x in 
        x)

        ;;
    esac
}

== stderr 0

== merged 277
f () 
{ 
    coproc X if a; then
        b;
    fi;
    coproc COPROC for i in 1;
    do
        :;
    done;
    coproc COPROC [[ -n x ]];
    coproc COPROC (( x ));
    coproc N (( x ));
    coproc N [[ -n y ]];
    coproc COPROC case x in 
        x)

        ;;
    esac
}

== status 0
