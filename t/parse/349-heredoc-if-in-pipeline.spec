== stdout 128
f () 
{ 
    if a; then
        cat <<A
a
A

    fi | b
    while read l; do
        :;
    done <<B |
b
B
  cat
    echo end
}

== stderr 0

== merged 128
f () 
{ 
    if a; then
        cat <<A
a
A

    fi | b
    while read l; do
        :;
    done <<B |
b
B
  cat
    echo end
}

== status 0
