== stdout 555
<declare><-a><a=([0]="1" [1]="x y" [2]="" [3]="z")>
<declare><-a><a=([0]="1"><[1]="x><y"><[2]=""><[3]="z")>
<declare -a a='1'>
<declare -a a='x y'>
<declare><-A><h=([k]="v" ["a b"]="c d" )>
<k "v" "a b" "c d" >
<k><v><a b><c d>
<0 "1" 1 "x y" 2 "" 3 "z">
<0><1><1><x y><2><><3><z>
<0 "1" 1 "x y" 2 "" 3 "z">
<'1'><'x y'><''><'z'>
<a><a><a><a><A><A>
<x y><><1><x y><><z><x y  z>
<0><1><2><3><k a b><k><a b>
<b><c><b><c><a><b>
<>< y><><><1 Q y  z><1><X Y><><Z><4><3><2><1>
< y><1><set><unset><alt>
<a><h>
<x y>
<z><z><d>
<><nope>
<zero><1><1>
<1><x y><><z>

== stderr 38
bash: line 12: a[7]: unbound variable

== merged 593
<declare><-a><a=([0]="1" [1]="x y" [2]="" [3]="z")>
<declare><-a><a=([0]="1"><[1]="x><y"><[2]=""><[3]="z")>
<declare -a a='1'>
<declare -a a='x y'>
<declare><-A><h=([k]="v" ["a b"]="c d" )>
<k "v" "a b" "c d" >
<k><v><a b><c d>
<0 "1" 1 "x y" 2 "" 3 "z">
<0><1><1><x y><2><><3><z>
<0 "1" 1 "x y" 2 "" 3 "z">
<'1'><'x y'><''><'z'>
<a><a><a><a><A><A>
<x y><><1><x y><><z><x y  z>
<0><1><2><3><k a b><k><a b>
<b><c><b><c><a><b>
<>< y><><><1 Q y  z><1><X Y><><Z><4><3><2><1>
< y><1><set><unset><alt>
<a><h>
<x y>
<z><z><d>
<><nope>
<zero><1><1>
<1><x y><><z>
bash: line 12: a[7]: unbound variable

== status 127
