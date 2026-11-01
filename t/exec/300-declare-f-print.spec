== stdout 360
simple () 
{ 
    echo hi
}
multi () 
{ 
    local x=1;
    if [ "$x" = 1 ]; then
        echo one;
    fi;
    for i in a b;
    do
        echo $i;
    done
}
kw () 
{ 
    echo kw
}
sub () 
{ 
    ( cd /tmp;
    echo "$PWD" )
}
redir () 
{ 
    echo out
} > /dev/null
pipe () 
{ 
    echo a | while read l; do
        echo "$l";
    done && true || false
}

== stderr 0

== merged 360
simple () 
{ 
    echo hi
}
multi () 
{ 
    local x=1;
    if [ "$x" = 1 ]; then
        echo one;
    fi;
    for i in a b;
    do
        echo $i;
    done
}
kw () 
{ 
    echo kw
}
sub () 
{ 
    ( cd /tmp;
    echo "$PWD" )
}
redir () 
{ 
    echo out
} > /dev/null
pipe () 
{ 
    echo a | while read l; do
        echo "$l";
    done && true || false
}

== status 0
