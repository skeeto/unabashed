== stdout 137
f () 
{ 
    x=$(e hi);
    y=`e there`;
    z=${ e funsub; };
    cat <<E
body
E

    echo $(c2 F
fb
F)
    { 
        echo grp
    }
}

== stderr 0

== merged 137
f () 
{ 
    x=$(e hi);
    y=`e there`;
    z=${ e funsub; };
    cat <<E
body
E

    echo $(c2 F
fb
F)
    { 
        echo grp
    }
}

== status 0
