== stdout 190
f () 
{ 
    coproc foo bar;
    coproc N ((x));
    coproc COPROC [[ -n a ]];
    coproc N [[ -n a ]];
    coproc N if a; then
        b;
    fi;
    coproc a=1 cmd;
    coproc cmd > out
}

== stderr 0

== merged 190
f () 
{ 
    coproc foo bar;
    coproc N ((x));
    coproc COPROC [[ -n a ]];
    coproc N [[ -n a ]];
    coproc N if a; then
        b;
    fi;
    coproc a=1 cmd;
    coproc cmd > out
}

== status 0
