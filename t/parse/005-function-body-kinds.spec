== stdout 455
a () 
{ 
    if true; then
        echo a;
    fi
}
b () 
{ 
    while false; do
        echo b;
    done
}
c () 
{ 
    until true; do
        echo c;
    done
}
d () 
{ 
    for x in 1 2;
    do
        echo $x;
    done
}
e () 
{ 
    case $1 in 
        *)
            echo e
        ;;
    esac
}
g () 
{ 
    [[ -n $1 ]]
}
h () 
{ 
    (( x += 1 ))
}
i () 
{ 
    ( echo sub )
}
j () 
{ 
    for ((k=0; k<2; k++))
    do
        echo $k;
    done
}

== stderr 0

== merged 455
a () 
{ 
    if true; then
        echo a;
    fi
}
b () 
{ 
    while false; do
        echo b;
    done
}
c () 
{ 
    until true; do
        echo c;
    done
}
d () 
{ 
    for x in 1 2;
    do
        echo $x;
    done
}
e () 
{ 
    case $1 in 
        *)
            echo e
        ;;
    esac
}
g () 
{ 
    [[ -n $1 ]]
}
h () 
{ 
    (( x += 1 ))
}
i () 
{ 
    ( echo sub )
}
j () 
{ 
    for ((k=0; k<2; k++))
    do
        echo $k;
    done
}

== status 0
