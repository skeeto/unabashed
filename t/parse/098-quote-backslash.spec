== stdout 89
f () 
{ 
    echo \a \$x \" \' \\ \* \  \; \& \| \< \> \( \) \{ \};
    echo a\ b c\\d
}

== stderr 0

== merged 89
f () 
{ 
    echo \a \$x \" \' \\ \* \  \; \& \| \< \> \( \) \{ \};
    echo a\ b c\\d
}

== status 0
