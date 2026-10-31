== stdout 100
f () 
{ 
    echo ${ cat <<E
x
E

 };
    echo ${ cat <<-E
y
E

 };
    echo "${ cat <<E
w
E

 }"
}

== stderr 256
bash: line 4: warning: here-document at line 2 delimited by end-of-file (wanted `E')
bash: line 7: warning: here-document at line 5 delimited by end-of-file (wanted `E')
bash: line 10: warning: here-document at line 8 delimited by end-of-file (wanted `E')

== merged 356
bash: line 4: warning: here-document at line 2 delimited by end-of-file (wanted `E')
bash: line 7: warning: here-document at line 5 delimited by end-of-file (wanted `E')
bash: line 10: warning: here-document at line 8 delimited by end-of-file (wanted `E')
f () 
{ 
    echo ${ cat <<E
x
E

 };
    echo ${ cat <<-E
y
E

 };
    echo "${ cat <<E
w
E

 }"
}

== status 0
