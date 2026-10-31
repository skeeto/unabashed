== stdout 71
a () 
{ 
    echo a
}
a () 
{ 
    echo a
}
b () 
{ 
    echo b
}
done

== stderr 0

== merged 71
a () 
{ 
    echo a
}
a () 
{ 
    echo a
}
b () 
{ 
    echo b
}
done

== status 1
