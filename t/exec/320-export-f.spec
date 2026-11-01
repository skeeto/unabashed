== stdout 81
status 0
f () 
{ 
    echo f
}
declare -fx f
declare -fx f
status 1
status 0
end

== stderr 45
bash: line 7: export: nosuch: not a function

== merged 126
status 0
f () 
{ 
    echo f
}
declare -fx f
declare -fx f
bash: line 7: export: nosuch: not a function
status 1
status 0
end

== status 0
