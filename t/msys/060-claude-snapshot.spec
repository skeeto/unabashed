== stdout 126
x y z
greet () 
{ 
    printf '[%s]\n' "$@"
}
argv[0] = <@WORK@\bin\argv.exe>
ll-alias
rg 3: -n pat file
[a b]
[c]
3 function

== stderr 0

== merged 126
x y z
greet () 
{ 
    printf '[%s]\n' "$@"
}
argv[0] = <@WORK@\bin\argv.exe>
ll-alias
rg 3: -n pat file
[a b]
[c]
3 function

== status 0
