== stdout 151
f () 
{ 
    echo @(a|*(b|+(c))) x!(y)z ?(\)) @(a\|b) "@("x")" @(a'|'b) +($(echo d));
    case $x in 
        !(*(a)|b) | @(c))

        ;;
    esac
}

== stderr 0

== merged 151
f () 
{ 
    echo @(a|*(b|+(c))) x!(y)z ?(\)) @(a\|b) "@("x")" @(a'|'b) +($(echo d));
    case $x in 
        !(*(a)|b) | @(c))

        ;;
    esac
}

== status 0
