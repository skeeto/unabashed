== stdout 34
f () 
{ 
    :
} <<$x
body
$y
$x


== stderr 86
bash: line 3: warning: here-document at line 1 delimited by end-of-file (wanted `$x')

== merged 120
bash: line 3: warning: here-document at line 1 delimited by end-of-file (wanted `$x')
f () 
{ 
    :
} <<$x
body
$y
$x


== status 0
