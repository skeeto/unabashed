== stdout 132
a.b
c-d
e:f
9x
g_h
declare -f 9x
declare -f a.b
declare -f c-d
declare -f e:f
declare -f g_h
c-d () 
{ 
    echo "c-d"
}
status 127

== stderr 38
bash: line 10: a.b: command not found

== merged 170
a.b
c-d
e:f
9x
g_h
declare -f 9x
declare -f a.b
declare -f c-d
declare -f e:f
declare -f g_h
c-d () 
{ 
    echo "c-d"
}
bash: line 10: a.b: command not found
status 127

== status 0
