== stdout 23
status 139
status 139 

== stderr 122
bash: line 5: 131075 Segmentation fault         ( trap 'echo exit trap' EXIT; function f () 
{ 
    f
}; f ) 2> /dev/null

== merged 145
bash: line 5: 131075 Segmentation fault         ( trap 'echo exit trap' EXIT; function f () 
{ 
    f
}; f ) 2> /dev/null
status 139
status 139 

== status 0
