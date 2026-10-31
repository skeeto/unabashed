== stdout 56
f () 
{ 
    echo '\q\z\ ' '"?' '\\' 'a\' '\%' '\N\k'
}

== stderr 0

== merged 56
f () 
{ 
    echo '\q\z\ ' '"?' '\\' 'a\' '\%' '\N\k'
}

== status 0
