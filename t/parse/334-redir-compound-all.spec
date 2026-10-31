== stdout 243
f () 
{ 
    ( a ) 3>&- 4>&5- <> rw {fd}>> log;
    { 
        a
    } 0<&3 1>&- 2>&1 &> both &>> both2;
    while a; do
        b;
    done {v}<<< str >| clob;
    case x in 
        x)

        ;;
    esac 0>&- 1>&-;
    [[ -n x ]] {w}>&2
}

== stderr 0

== merged 243
f () 
{ 
    ( a ) 3>&- 4>&5- <> rw {fd}>> log;
    { 
        a
    } 0<&3 1>&- 2>&1 &> both &>> both2;
    while a; do
        b;
    done {v}<<< str >| clob;
    case x in 
        x)

        ;;
    esac 0>&- 1>&-;
    [[ -n x ]] {w}>&2
}

== status 0
