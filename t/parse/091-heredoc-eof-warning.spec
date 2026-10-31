== stdout 59
f () 
{ 
    cat
} <<EOF
body line
no terminator here
EOF


== stderr 87
bash: line 3: warning: here-document at line 1 delimited by end-of-file (wanted `EOF')

== merged 146
bash: line 3: warning: here-document at line 1 delimited by end-of-file (wanted `EOF')
f () 
{ 
    cat
} <<EOF
body line
no terminator here
EOF


== status 0
