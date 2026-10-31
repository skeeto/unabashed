== stdout 237
f () 
{ 
    echo ${ cat <<E
x
E

 };
    echo ${ { echo a; }; };
    echo ${ sleep 1 & };
    echo ${ { echo b; } & };
    echo ${|REPLY=$(cat <<E
y
E
); };
    echo ${ cat <<E
z
E
; };
    echo ${ echo tab; };
    echo ${ echo nl
 }
}

== stderr 75
bash: line 13: warning: command substitution: 1 unterminated here-document

== merged 312
bash: line 13: warning: command substitution: 1 unterminated here-document
f () 
{ 
    echo ${ cat <<E
x
E

 };
    echo ${ { echo a; }; };
    echo ${ sleep 1 & };
    echo ${ { echo b; } & };
    echo ${|REPLY=$(cat <<E
y
E
); };
    echo ${ cat <<E
z
E
; };
    echo ${ echo tab; };
    echo ${ echo nl
 }
}

== status 0
