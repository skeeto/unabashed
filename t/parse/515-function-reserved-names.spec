== stdout 108
do () 
{ 
    echo do
}
done () 
{ 
    ( echo done )
}
esac () 
{ 
    echo esac
}
in () 
{ 
    echo in
}

== stderr 0

== merged 108
do () 
{ 
    echo do
}
done () 
{ 
    ( echo done )
}
esac () 
{ 
    echo esac
}
in () 
{ 
    echo in
}

== status 0
