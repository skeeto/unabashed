== stdout 89
start
same line
f () 
{ 
    cat
} <<EOF
heredoc runs to the end
echo not a command
EOF


== stderr 87
bash: line 4: warning: here-document at line 2 delimited by end-of-file (wanted `EOF')

== merged 176
start
bash: line 4: warning: here-document at line 2 delimited by end-of-file (wanted `EOF')
same line
f () 
{ 
    cat
} <<EOF
heredoc runs to the end
echo not a command
EOF


== status 0
