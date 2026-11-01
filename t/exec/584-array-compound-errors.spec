== stdout 93
next
next2
next3
declare -a h=([0]="x")
declare -A g=([0]="str" )
1
1
declare -a b=([0]="1")

== stderr 616
bash: line 1: ass: x: must use subscript when assigning associative array
bash: line 3: []=v: bad array subscript
bash: line 5: [-5]=x: bad array subscript
bash: line 7: d: 'x': must use subscript when assigning associative array
bash: line 8: q: 'x': must use subscript when assigning associative array
bash: line 9: [@]=1: cannot assign to non-numeric index
bash: line 10: [-1]=1: bad array subscript
bash: line 11: declare: h: cannot convert indexed to associative array
bash: line 12: declare: g: cannot convert associative to indexed array
bash: line 13: declare: b: cannot convert indexed to associative array

== merged 709
bash: line 1: ass: x: must use subscript when assigning associative array
next
bash: line 3: []=v: bad array subscript
next2
bash: line 5: [-5]=x: bad array subscript
next3
bash: line 7: d: 'x': must use subscript when assigning associative array
bash: line 8: q: 'x': must use subscript when assigning associative array
bash: line 9: [@]=1: cannot assign to non-numeric index
bash: line 10: [-1]=1: bad array subscript
bash: line 11: declare: h: cannot convert indexed to associative array
declare -a h=([0]="x")
declare -A g=([0]="str" )
bash: line 12: declare: g: cannot convert associative to indexed array
1
bash: line 13: declare: b: cannot convert indexed to associative array
1
declare -a b=([0]="1")

== status 0
