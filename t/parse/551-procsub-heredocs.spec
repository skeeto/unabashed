== stdout 132
f () 
{ 
    cat <(cat <<E
x
E
) >(tr a b <<F
f
F
);
    diff <(cat <<A
a
A
) <(cat <<B
b
B
);
    cat < <(cat <<C
c
C
) <<D
d
D

}

== stderr 149
bash: line 9: warning: command substitution: 1 unterminated here-document
bash: line 11: warning: command substitution: 1 unterminated here-document

== merged 281
bash: line 9: warning: command substitution: 1 unterminated here-document
bash: line 11: warning: command substitution: 1 unterminated here-document
f () 
{ 
    cat <(cat <<E
x
E
) >(tr a b <<F
f
F
);
    diff <(cat <<A
a
A
) <(cat <<B
b
B
);
    cat < <(cat <<C
c
C
) <<D
d
D

}

== status 0
