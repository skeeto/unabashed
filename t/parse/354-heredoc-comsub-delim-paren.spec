== stdout 111
f () 
{ 
    x=$(cat <<A
body
A
);
    y=$(cat <<B
body
B
) more;
    z=${ cat <<C
body
C

 };
    echo done
}

== stderr 256
bash: line 4: warning: here-document at line 2 delimited by end-of-file (wanted `A')
bash: line 7: warning: here-document at line 5 delimited by end-of-file (wanted `B')
bash: line 10: warning: here-document at line 8 delimited by end-of-file (wanted `C')

== merged 367
bash: line 4: warning: here-document at line 2 delimited by end-of-file (wanted `A')
bash: line 7: warning: here-document at line 5 delimited by end-of-file (wanted `B')
bash: line 10: warning: here-document at line 8 delimited by end-of-file (wanted `C')
f () 
{ 
    x=$(cat <<A
body
A
);
    y=$(cat <<B
body
B
) more;
    z=${ cat <<C
body
C

 };
    echo done
}

== status 0
