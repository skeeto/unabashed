== stdout 197
f () 
{ 
    echo ${x:-${y:-${z:-def}}};
    echo ${x:-"quoted $y"} ${x:-'single'} ${x/"$p"/$r};
    echo "${x:-a b}" "${x#"$p"}" ${x:-$(cmd)} ${x:-`cmd`};
    echo ${x//\//_} ${x/\}/r} ${x:-\}}
}

== stderr 0

== merged 197
f () 
{ 
    echo ${x:-${y:-${z:-def}}};
    echo ${x:-"quoted $y"} ${x:-'single'} ${x/"$p"/$r};
    echo "${x:-a b}" "${x#"$p"}" ${x:-$(cmd)} ${x:-`cmd`};
    echo ${x//\//_} ${x/\}/r} ${x:-\}}
}

== status 0
