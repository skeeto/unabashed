== stdout 120
f () 
{ 
    echo "${x:1}" "${a[k]}" ${a['k']} "${x-it's}";
    echo "$[ 1 ]" "$(echo "${x:-	}")" "${x:-$(echo '	')}"
}

== stderr 0

== merged 120
f () 
{ 
    echo "${x:1}" "${a[k]}" ${a['k']} "${x-it's}";
    echo "$[ 1 ]" "$(echo "${x:-	}")" "${x:-$(echo '	')}"
}

== status 0
