== stdout 96
status 1
original
unset status 1
original
status 1
f () 
{ 
    echo original
}
declare -fr f
f

== stderr 138
bash: line 3: f: readonly function
bash: line 6: unset: f: cannot unset: readonly function
bash: line 9: readonly: nosuch: not a function

== merged 234
bash: line 3: f: readonly function
status 1
original
bash: line 6: unset: f: cannot unset: readonly function
unset status 1
original
bash: line 9: readonly: nosuch: not a function
status 1
f () 
{ 
    echo original
}
declare -fr f
f

== status 0
