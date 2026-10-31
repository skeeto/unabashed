== stdout 143
f () 
{ 
    echo $(echo a);
    echo $(echo b);
    echo $(echo c);
    echo "$(echo d)";
    echo $(echo e#notcomment);
    echo $(echo f)
}

== stderr 0

== merged 143
f () 
{ 
    echo $(echo a);
    echo $(echo b);
    echo $(echo c);
    echo "$(echo d)";
    echo $(echo e#notcomment);
    echo $(echo f)
}

== status 0
