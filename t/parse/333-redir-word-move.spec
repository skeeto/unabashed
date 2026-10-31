== stdout 159
f () 
{ 
    a 1>&"x"- 0<&'3'- 2>&3x- 4<&$y-;
    a x 1>&-;
    a b 1>&-;
    a - c 1>&-;
    a >&'-' <&"-";
    a {1v} {v} {arr[1]}> f {arr[$i]}< g > h > k
}

== stderr 0

== merged 159
f () 
{ 
    a 1>&"x"- 0<&'3'- 2>&3x- 4<&$y-;
    a x 1>&-;
    a b 1>&-;
    a - c 1>&-;
    a >&'-' <&"-";
    a {1v} {v} {arr[1]}> f {arr[$i]}< g > h > k
}

== status 0
