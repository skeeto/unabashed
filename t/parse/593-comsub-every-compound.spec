== stdout 830
f () 
{ 
    x=$(if a; then
    b;
else
    if c; then
        d;
    else
        e;
    fi;
fi; while f; do
    g;
done; until h; do
    i;
done);
    x=$(for j in k l;
do
    m;
done; for ((n=0; n<1; n++))
do
    o;
done; select p in q;
do
    r;
done);
    x=$(case s in t)
        u
    ;;
    v | w)
        x
    ;&
    y)
        z
    ;;&
esac; { aa; bb; }; ( cc; dd ));
    x=$([[ -n ee && ! -n ff || ( -n gg ) ]]; (( hh )); time ii | jj; ! kk 2>&1 | ll);
    x=$(function mm () 
{ 
    nn
}; function oo () 
{ 
    ( pp )
}; function qq () 
{ 
    [[ -n rr ]]
}; function ss () 
{ 
    (( tt ))
});
    x=$(coproc uu; coproc VV { ww; }; xx & yy && zz || aaa; bbb &);
    x=$(cmd > out 2>&1 < in 3<> rw 4>&- >| clob &> both &>> app 0<&5 {fd}> x);
    x=$(cmd <<E <<-F <<< here
e
E
f
F
);
    x=$(if a; then
    b;
fi)
}

== stderr 0

== merged 830
f () 
{ 
    x=$(if a; then
    b;
else
    if c; then
        d;
    else
        e;
    fi;
fi; while f; do
    g;
done; until h; do
    i;
done);
    x=$(for j in k l;
do
    m;
done; for ((n=0; n<1; n++))
do
    o;
done; select p in q;
do
    r;
done);
    x=$(case s in t)
        u
    ;;
    v | w)
        x
    ;&
    y)
        z
    ;;&
esac; { aa; bb; }; ( cc; dd ));
    x=$([[ -n ee && ! -n ff || ( -n gg ) ]]; (( hh )); time ii | jj; ! kk 2>&1 | ll);
    x=$(function mm () 
{ 
    nn
}; function oo () 
{ 
    ( pp )
}; function qq () 
{ 
    [[ -n rr ]]
}; function ss () 
{ 
    (( tt ))
});
    x=$(coproc uu; coproc VV { ww; }; xx & yy && zz || aaa; bbb &);
    x=$(cmd > out 2>&1 < in 3<> rw 4>&- >| clob &> both &>> app 0<&5 {fd}> x);
    x=$(cmd <<E <<-F <<< here
e
E
f
F
);
    x=$(if a; then
    b;
fi)
}

== status 0
