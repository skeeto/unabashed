== stdout 140
f () 
{ 
    echo ${ cmd; } ${ a; b; } "${ cmd; }";
    echo ${ cmd; } ${ multi
line
 };
    echo ${ if a; then
    b;
fi; } ${ { x; }; }
}

== stderr 0

== merged 140
f () 
{ 
    echo ${ cmd; } ${ a; b; } "${ cmd; }";
    echo ${ cmd; } ${ multi
line
 };
    echo ${ if a; then
    b;
fi; } ${ { x; }; }
}

== status 0
