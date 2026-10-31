== stdout 75
f () 
{ 
    cat $(a; b) "$(c
d)" <<A
x
A

    echo $(cat <<B
b
B
) tail
}

== stderr 74
bash: line 6: warning: command substitution: 1 unterminated here-document

== merged 149
bash: line 6: warning: command substitution: 1 unterminated here-document
f () 
{ 
    cat $(a; b) "$(c
d)" <<A
x
A

    echo $(cat <<B
b
B
) tail
}

== status 0
