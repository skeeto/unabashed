== stdout 122
f () 
{ 
    echo @(a|b) !(*.o) +(x) *(y) ?(z) pre@(a|b)post;
    echo !(@(a|b)|c) "@(not)" \@\(no\);
    rm -- !(keep)
}

== stderr 0

== merged 122
f () 
{ 
    echo @(a|b) !(*.o) +(x) *(y) ?(z) pre@(a|b)post;
    echo !(@(a|b)|c) "@(not)" \@\(no\);
    rm -- !(keep)
}

== status 0
