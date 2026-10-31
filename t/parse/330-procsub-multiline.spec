== stdout 150
f () 
{ 
    diff <(x
y) >(if a; then
    b;
fi);
    cat <(case q in q)
        r
    ;;
esac) <(while a; do
    b;
done);
    cat <(cat <<A
p
A
)
}

== stderr 0

== merged 150
f () 
{ 
    diff <(x
y) >(if a; then
    b;
fi);
    cat <(case q in q)
        r
    ;;
esac) <(while a; do
    b;
done);
    cat <(cat <<A
p
A
)
}

== status 0
