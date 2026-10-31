== stdout 80
f () 
{ 
    x=${ cat <<A |
a
A
  wc; };
    y=${ cat <<B && 
b
B
 echo ok
 }
}

== stderr 74
bash: line 2: warning: command substitution: 1 unterminated here-document

== merged 154
bash: line 2: warning: command substitution: 1 unterminated here-document
f () 
{ 
    x=${ cat <<A |
a
A
  wc; };
    y=${ cat <<B && 
b
B
 echo ok
 }
}

== status 0
