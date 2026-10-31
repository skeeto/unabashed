== stdout 237
f () 
{ 
    coproc cmd arg;
    coproc NAME { 
        cmd
    };
    coproc COPROC ( cmd );
    coproc NAME2 ( a;
    b );
    coproc COPROC { 
        a;
        b
    };
    coproc COPROC while read l; do
        echo $l;
    done
}

== stderr 0

== merged 237
f () 
{ 
    coproc cmd arg;
    coproc NAME { 
        cmd
    };
    coproc COPROC ( cmd );
    coproc NAME2 ( a;
    b );
    coproc COPROC { 
        a;
        b
    };
    coproc COPROC while read l; do
        echo $l;
    done
}

== status 0
